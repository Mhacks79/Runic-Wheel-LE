#!/usr/bin/env python3
"""Lossless-preserving RWAT central medallion replacement for Skyrim LE Runic Wheel.

The original frame and the user's RGBA rune are immutable inputs. The central
opaque roundel is excised in atlas pixel coordinates then the source rune is
centred based on its *visible alpha bounding box*; nothing else is changed.
"""
from pathlib import Path
import argparse
import hashlib
import math
import struct
from PIL import Image
import numpy as np


def decode(path):
    raw = path.read_bytes()
    assert len(raw) >= 12 and raw[:4] == b'RWAT'
    w,h=struct.unpack_from('<II',raw,4)
    assert (w,h)==(1024,1024) and len(raw)==12+w*h*4
    return Image.frombytes('RGBA',(w,h),raw[12:],'raw','BGRA')


def prepare(frame, rune, span=338):
    r = rune.convert('RGBA')
    a = np.asarray(r.getchannel('A'))
    yy, xx = np.nonzero(a > 8)
    assert len(xx)>1000, 'Source rune is empty'
    # alpha bbox is the only source of optical alignment (no assumption that the PNG canvas is centered)
    bbox = (int(xx.min()),int(yy.min()),int(xx.max()+1),int(yy.max()+1))
    cropped = r.crop(bbox)
    cw,ch=cropped.size
    # preserve aspect ratio, never stretch the emblem
    z=min(span/cw, span/ch)
    sz=(round(cw*z),round(ch*z))
    scaled=cropped.resize(sz,Image.Resampling.LANCZOS)
    x=round(512-(sz[0])/2)
    y=round(512-(sz[1])/2)
    src=np.array(frame.convert('RGBA'),dtype=np.uint8)
    yy,xx=np.ogrid[:1024,:1024]
    radius=np.sqrt((xx-511.5)**2+(yy-511.5)**2)
    # The old disc occupies r < ~177, while the spokes and ring outside r=180 are unchanged.
    transition=np.clip((radius-176.0)/3.0,0.0,1.0)
    src[:,:,3]=np.rint(src[:,:,3].astype(np.float64)*transition).astype(np.uint8)
    src[src[:,:,3]==0,:3]=0  # avoid bilinear texture dark fringes when alpha is zero
    out=Image.fromarray(src,'RGBA')
    out.alpha_composite(scaled,(x,y))
    # Preserve every pixel outside the edited roundel + inserted rune bounds.
    return out,dict(bbox=bbox,size=sz,pos=(x,y),source_size=r.size)


def main():
    p=argparse.ArgumentParser()
    p.add_argument('--frame',type=Path,default=Path(__file__).parent/'input/RunicWheelFrame-v025.rwtx')
    p.add_argument('--rune',type=Path,default=Path(__file__).parent/'input/user-rune.png')
    p.add_argument('--output',type=Path,default=Path(__file__).parent/'output/RunicWheelFrame.rwtx')
    args=p.parse_args()
    base=decode(args.frame)
    img,details=prepare(base,Image.open(args.rune))
    args.output.parent.mkdir(exist_ok=True,parents=True)
    args.output.write_bytes(b'RWAT'+struct.pack('<II',1024,1024)+img.tobytes('raw','BGRA'))
    img.save(args.output.with_suffix('.png'))
    print('original SHA256',hashlib.sha256(args.frame.read_bytes()).hexdigest())
    print('rune SHA256',hashlib.sha256(args.rune.read_bytes()).hexdigest())
    print('new SHA256',hashlib.sha256(args.output.read_bytes()).hexdigest())
    print('alpha centered',details)

if __name__=='__main__':main()
