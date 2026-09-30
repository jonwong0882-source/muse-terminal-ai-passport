#!/usr/bin/env python3
"""Convert the user-supplied Muse avatar clip to Flash-only RGB565 frames."""
import hashlib
import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "assets/images/muse-avatar-source.mp4"
OUTPUT = ROOT / "assets/images/muse-avatar-rgb565.bin"
INFO = ROOT / "assets/images/muse-avatar-rgb565.json"
SIZE = 240
FPS = 4
FRAMES = 20
COMPACT = 100
RECORD = 164


def circle_portrait(square: bytes, size: int, background_rgb: tuple[int, int, int], inner_radius: int, outer_radius: int) -> bytes:
    """Use a fixed RGB565 mask to avoid LVGL's RAM-heavy corner clipping."""
    def rgb565(r: int, g: int, b: int) -> bytes:
        return (((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)).to_bytes(2, "little")

    background = rgb565(*background_rgb)
    border = rgb565(0xF1, 0xDE, 0xD2)
    result = bytearray(len(square))
    center = (size - 1) / 2
    for y in range(size):
        for x in range(size):
            distance2 = (x - center) ** 2 + (y - center) ** 2
            offset = (y * size + x) * 2
            result[offset:offset + 2] = (
                square[offset:offset + 2] if distance2 < inner_radius ** 2
                else border if distance2 < outer_radius ** 2 else background
            )
    return bytes(result)


def render(width: int, height: int, count: int, fps: int | None = None) -> bytes:
    filters = []
    if fps is not None:
        filters.append(f"fps={fps}")
    filters.append(f"scale={width}:{height}:flags=lanczos")
    result = subprocess.run(
        ["ffmpeg", "-hide_banner", "-loglevel", "error", "-i", str(SOURCE),
         "-an", "-vf", ",".join(filters), "-frames:v", str(count),
         "-pix_fmt", "rgb565le", "-f", "rawvideo", "-"],
        check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
    )
    expected = width * height * 2 * count
    if len(result.stdout) != expected:
        raise RuntimeError(f"Expected {expected} avatar bytes, received {len(result.stdout)}")
    return result.stdout


def main() -> None:
    if not SOURCE.is_file():
        raise SystemExit(f"Missing source: {SOURCE}")
    full = render(SIZE, SIZE, FRAMES, FPS)
    compact = circle_portrait(render(COMPACT, COMPACT, 1), COMPACT, (0xFF, 0xFF, 0xFF), 46, 49)
    recording = circle_portrait(render(RECORD, RECORD, 1), RECORD, (0x1B, 0x1D, 0x20), 78, 81)
    data = full + compact + recording
    OUTPUT.write_bytes(data)
    INFO.write_text(json.dumps({
        "source": SOURCE.name,
        "source_sha256": hashlib.sha256(SOURCE.read_bytes()).hexdigest(),
        "output_sha256": hashlib.sha256(data).hexdigest(),
        "format": "RGB565 little-endian",
        "full_frames": FRAMES,
        "full_size": [SIZE, SIZE],
        "full_fps": FPS,
        "compact_size": [COMPACT, COMPACT],
        "record_size": [RECORD, RECORD],
        "bytes": len(data),
    }, indent=2) + "\n")
    print(f"Muse avatar: {FRAMES} frames plus compact portrait, {len(data)} Flash bytes")


if __name__ == "__main__":
    main()
