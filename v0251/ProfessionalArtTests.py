from pathlib import Path
import struct
from PIL import Image
p=Path(__file__).resolve().parent
for file,w,h in [('RunicWheelIcons.rwtx',2048,1024),('RunicWheelFrame.rwtx',1024,1024),('RunicWheelFont.rwtx',1024,1024)]:
    raw=(p/file).read_bytes();assert raw[:4]==b'RWAT'
    assert raw[4:12]==struct.pack('<II',w,h)
    assert len(raw)==12+w*h*4
atlas=Image.open(p/'art/Icons-PRO-PREVIEW.png').convert('RGBA')
assert atlas.size==(2048,1024)
from collections import Counter
# Each category tile is wholly independent, with a safety gutter and nonempty centre.
for i in range(32):
    x,y=(i%8)*256,(i//8)*256
    im=atlas.crop((x,y,x+256,y+256)); a=im.getchannel('A')
    assert a.getpixel((0,0))==0 and a.getpixel((255,255))==0,i
    assert a.getpixel((128,128))>150,i
    assert im.getbbox() is not None
# Guard against accidental identical icons and poison/food/potion regression.
def digest_symbol(i):
    import hashlib
    tile=atlas.crop(((i%8)*256+65,(i//8)*256+65,(i%8)*256+191,(i//8)*256+191))
    return hashlib.sha256(tile.tobytes()).hexdigest()
assert len({digest_symbol(i) for i in range(32)})==32
s=(p/'RunicWheelV011.cpp').read_text()
assert 'g_iconIndices[wheel][sector][index]' in s
assert 'g_frameAtlas=read_texture' in s and 'g_fontAtlas=read_texture' in s
assert 'g_iconAtlas=read_texture' in s
assert 'professional_text(cx' in s and 'professional_text(x' in s
assert 'font_index' in s and 'utf8_decode' in s
assert '0xF1' in s and '0xE4' in s and '0xA8' in s
assert 'first-concept game icon atlas READY' not in s
# The pre-existing equipment routes are retained; new torch route is separately audited.
assert 'GameEquipSpell>(0x006EFAC0)' in s
assert 'RunicWheelLE: WEAPON equip request submitted' in s
assert 'RunicWheelLE: TORCH equip request submitted' in s
assert 'RunicWheelLE: consumable/ammo request submitted' in s
print('PASS: 32 isolated icons, independent category mapping, UTF-8 art, 3 BGRA atlases, preserved native action anchors')
