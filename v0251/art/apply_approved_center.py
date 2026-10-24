#!/usr/bin/env python3
"""Extract ONLY the fantasy medallion from the first user-approved AI-generated concept.
This is NOT a contact-sheet icon crop. The radial wedge/environment pixels are excluded.
"""
from pathlib import Path
import numpy as np, struct
from PIL import Image
P=Path(__file__).resolve().parent;root=P.parent
src=Image.open(P/'FIRST-APPROVED-WHEEL-REFERENCE.png').convert('RGBA')
assert src.size[0]>=1500 and src.size[1]>=900
# The only retained part is the central heraldic medallion, not screenshot backdrop.
cx,cy=825,405;radius=386
ref=src.crop((cx-radius,cy-radius,cx+radius,cy+radius)).resize((1024,1024),Image.Resampling.LANCZOS)
y,x=np.ogrid[:1024,:1024]
r=np.sqrt((x-512)**2+(y-512)**2)
mask=np.clip((158-r)/6,0,1)
a=np.asarray(ref).copy();a[:,:,3]=np.uint8(mask*255)
medallion=Image.fromarray(a,'RGBA')
framefile=root/'RunicWheelFrame.rwtx'
raw=framefile.read_bytes();assert raw[:12]==b'RWAT'+struct.pack('<II',1024,1024)
raw_pixels=np.frombuffer(raw[12:],dtype=np.uint8).reshape((1024,1024,4))
base=Image.fromarray(raw_pixels[:,:,[2,1,0,3]].copy(),'RGBA')
base.alpha_composite(medallion)
b=bytes(np.asarray(base)[:,:,[2,1,0,3]].tobytes())
framefile.write_bytes(b'RWAT'+struct.pack('<II',1024,1024)+b)
base.save(P/'RunicWheelFrame-PREVIEW.png')
base.save(P/'Frame-PRO-PREVIEW.png')
# Quantitative assertions: zero extracted outside central 158px in pre-overlay source.
assert medallion.getchannel('A').getpixel((0,0))==0
assert medallion.getchannel('A').getpixel((512,512))==255
print('PASS: approved original concept center only, all exterior/background pixels excluded')
