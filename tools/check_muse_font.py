#!/usr/bin/env python3
"""Check the generated inventory and fixed UI strings without optional modules."""
import hashlib,json,re
from pathlib import Path
root=Path(__file__).resolve().parents[1]
info=json.loads((root/'assets/fonts/muse-glyphs.json').read_text());codes=set(info['codepoints'])
assert hashlib.sha256((root/'assets/fonts/NotoSansCJKsc-Regular.otf').read_bytes()).hexdigest()==info['source_sha256']
# C comments can contain other languages; inspect string literals in actual UI.
required=set()
for name in ['muse_ui.c','muse_main.c']:
    source=(root/'main'/name).read_text()
    for literal in re.findall(r'"(?:[^"\\]|\\.)*"',source):
        for char in literal:
            if ord(char)>127:required.add(ord(char))
missing=required-codes
assert not missing, 'Missing fixed UI glyphs: '+','.join(f'U+{n:04X}' for n in sorted(missing))
assert 0x1f680 not in codes,'negative coverage control failed'
assert all(cp in codes for cp in map(ord,'中文繁體，。ABC123?'))
source=(root/'assets/fonts/muse_font_16.c').read_text()
assert 'muse_font_16' in source and '.bpp = 2' in source
print(f'Muse font inventory: PASS ({len(codes)} glyphs, {len(required)} fixed UI characters; emoji explicitly unsupported)')
