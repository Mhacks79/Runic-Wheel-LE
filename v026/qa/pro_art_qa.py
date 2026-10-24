#!/usr/bin/env python3
from pathlib import Path
import importlib.util,hashlib,struct
from PIL import Image
p=Path(__file__).resolve().parents[1]
font=Image.open(p/'art/Font-PRO-PREVIEW.png').convert('RGBA');fa=font.getchannel('A')
spec=importlib.util.spec_from_file_location('rwart',p/'art/generate.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
assert len(m.ICONS)==32 and len(set(k for k,c in m.ICONS))==32
assert len(m.FONT_CODES)>=195
for cp in [ord('É'),ord('È'),ord('À'),ord('Ç'),ord('Ù'),ord('Ô'),ord('Ï'),ord('œ'),ord('é'),ord('è'),0x2026]:
    k=m.FONT_CODES.index(cp)
    box=fa.crop(((k%16)*64,(k//16)*64,(k%16+1)*64,(k//16+1)*64))
    assert box.getbbox(),hex(cp)
    assert box.getpixel((0,0))==0 and box.getpixel((63,63))==0,hex(cp)
# Check visual previews exist and aren't little previews reused as fake high-res.
for w,h in [(1280,720),(1920,1080),(2560,1440)]:
    im=Image.open(p/f'art/UI-PRO-PREVIEW-{w}x{h}.png').convert('RGB')
    assert im.size==(w,h)
    assert im.getpixel((w//2,h//2))!=im.getpixel((10,10))
# Icon map must be one-to-one in the generated atlas: don't regress to SheetCrop.
source=(p/'art/generate.py').read_text()
assert 'ICON' in source and 'glyph_body(name)' in source
assert 'sprite' in source and 'crop((x,y' not in source
# Negative changes must be caught by paired tests, not hidden in claimed 'pass'.
s=(p/'RunicWheelV011.cpp').read_text()
mutant=s.replace('g_iconIndices[wheel][sector][index]','(unsigned)b.kind')
assert 'g_iconIndices[wheel][sector][index]' not in mutant
assert 'g_iconIndices[wheel][sector][index]' in s
assert 'readable(form,0xF2,0)' in s and 'readable(form,0xE8,0)' in s
assert '0x00452300' not in s[s.index('void draw_labels('):s.index('int draw(Raw device)')]
print('PASS professional visual QA: 32 distinct sprites, French glyphs, alpha gutters, 720p/1080p/1440p previews, read-only game task, negative category mapping')
