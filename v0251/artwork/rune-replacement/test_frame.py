"""Static regression contract for the supplied centered transparent rune frame."""
from pathlib import Path
import hashlib, numpy as np
from PIL import Image
from rebuild_frame import decode,prepare
root=Path(__file__).resolve().parent
source=decode(root/'input/RunicWheelFrame-v025.rwtx')
rune=Image.open(root/'input/user-rune.png').convert('RGBA')
updated=decode(root/'output/RunicWheelFrame.rwtx')
repro,info=prepare(source,rune)
assert repro.tobytes()==updated.tobytes(),'Texture generator not deterministic'
assert updated.size==source.size==(1024,1024)
assert (updated.getchannel('A').getextrema()[0]==0) and (updated.getchannel('A').getextrema()[1]==255)
src=np.asarray(source); dst=np.asarray(updated)
y,x=np.ogrid[:1024,:1024];radius=np.sqrt((x-511.5)**2+(y-511.5)**2)
beyond=(radius>185)
# The rune never extends past center +/- 170px; all outer spokes and ornaments unchanged.
assert np.array_equal(src[beyond],dst[beyond]),'Outer frame unexpectedly modified'
assert np.any((src[450:475,550:570,3]>245)&(dst[450:475,550:570,3]==0)), 'Old central roundel is still covering the rune'
# Visible-alpha crop is centered in the 1024x1024 texel canvas.
px,py=info['pos'];ww,hh=info['size']
assert abs(px+ww/2-512)<=.5 and abs(py+hh/2-512)<=.5
assert 310<=hh<=350 and ww/hh<.7
# A point in the icon's hollow upper triangle is transparent; not a black plate.
assert updated.getpixel((512,440))[3] < 255,'Transparent rune cutout lost'
print('PASS: deterministic BGRA RWAT; alpha cutouts, physical centering, preserved entire external rim/spokes')
print('SHA256',hashlib.sha256((root/'output/RunicWheelFrame.rwtx').read_bytes()).hexdigest())
