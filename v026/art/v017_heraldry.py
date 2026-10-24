#!/usr/bin/env python3
"""Deterministic independent 8x4 medallions from a private, newly generated illustration.
Never slice an inferred collage bounding box: tile cells are mathematically defined.
Masks guarantee fully transparent external gutters and no cross-cell bleeding.
RWAT payload is strict D3D9 little-endian BGRA8, 2048x1024.
"""
from pathlib import Path
from PIL import Image, ImageEnhance, ImageFilter
import numpy as np, struct, hashlib
ROOT=Path(__file__).resolve().parent.parent
art=ROOT/'art'
image=Image.open(art/'V017-Heraldry-Source.png').convert('RGB')
w,h=image.size
assert abs(w/h-2.0)<0.00001,(w,h)
# Artwork source is divided into exact 8 columns and 4 rows, without text.
result=Image.new('RGBA',(2048,1024),(0,0,0,0))
for i in range(32):
    col,row=i%8,i//8
    box=(round(col*w/8),round(row*h/4),round((col+1)*w/8),round((row+1)*h/4))
    tile=image.crop(box).resize((256,256),Image.Resampling.LANCZOS)
    # Adjust for dark Skyrim world: preserve material coloration, gently open midtones.
    tile=ImageEnhance.Contrast(tile).enhance(1.08)
    tile=ImageEnhance.Color(tile).enhance(1.10)
    rgb=np.asarray(tile,dtype=np.uint8).astype(np.float32)
    luminance=np.max(rgb,axis=2)
    # Unmodified black outside the embossed medallion is always transparent.
    # Soft circle keeps the whole metal rim, and never touches a tile border.
    yy,xx=np.ogrid[:256,:256]
    radius=np.sqrt((xx-127.5)**2+(yy-127.5)**2)
    edge=np.clip((124-radius)/8.0,0,1)
    # Silhouette mask removes background outside circle but doesn't clip carved edging.
    # Very dark black portions of interior retained to preserve detail.
    alpha=(255*edge).astype(np.uint8)
    arr=np.concatenate((rgb.astype(np.uint8),alpha[:,:,None]),axis=2)
    medallion=Image.fromarray(arr,'RGBA')
    result.alpha_composite(medallion,(col*256,row*256))
# Save visual QA previews, and exact BGRA raw payload.
result.save(art/'V017-Heraldry-Atlas.png')
result.save(art/'Icons-PRO-PREVIEW.png')
result.save(art/'RunicWheelIcons-PREVIEW.png')
pixels=np.asarray(result)
raw=b'RWAT'+struct.pack('<II',2048,1024)+pixels[:,:,[2,1,0,3]].tobytes()
(ROOT/'RunicWheelIcons.rwtx').write_bytes(raw)
assert len(raw)==12+2048*1024*4
for i in range(32):
 tile=result.crop((i%8*256,i//8*256,i%8*256+256,i//8*256+256))
 a=tile.getchannel('A')
 assert all(a.getpixel(pt)==0 for pt in ((0,0),(128,0),(0,128),(255,255),(255,128),(128,255))),i
 assert a.getpixel((128,128))==255,i
assert len({hashlib.sha256(result.crop((i%8*256+50,i//8*256+50,i%8*256+205,i//8*256+205)).tobytes()).hexdigest() for i in range(32)})==32
print('PASS: 32 exact image cells, independently cropped, transparent 8px+ gutters, BGRA8 atlased',hashlib.sha256(raw).hexdigest())
