#!/usr/bin/env python3
"""Generate the pinned 16px 2bpp Muse font. Requires fonttools and lv_font_conv 1.5.3."""
import hashlib,json,subprocess,sys
from pathlib import Path
from fontTools.ttLib import TTFont
root=Path(__file__).resolve().parents[1]
font=root/'assets/fonts/NotoSansCJKsc-Regular.otf'
cmap=TTFont(font).getBestCmap()
codes=sorted(cp for cp in cmap if 0x20<=cp<=0x7e or 0xa0<=cp<=0xff or 0x2000<=cp<=0x206f or 0x3000<=cp<=0x303f or 0x4e00<=cp<=0x9fff or 0xff00<=cp<=0xffef)
(root/'assets/fonts/muse-glyphs.json').write_text(json.dumps({'source_sha256':hashlib.sha256(font.read_bytes()).hexdigest(),'converter':'1.5.3','size':16,'bpp':2,'codepoints':codes}))
subprocess.run([sys.argv[1] if len(sys.argv)>1 else 'lv_font_conv','--font',str(font),'--symbols',''.join(map(chr,codes)),'--size','16','--bpp','2','--no-compress','--no-kerning','--format','lvgl','--lv-font-name','muse_font_16','--lv-include','lvgl.h','-o',str(root/'assets/fonts/muse_font_16.c')],check=True)
print(f'Generated {len(codes)} glyphs')
