#!/usr/bin/env python3
"""Strictly offline preview: follows v0.15 DX9 placement, does not claim in-game render."""
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
import numpy as np, math
P=Path(__file__).resolve().parent.parent;A=P/'art'
ICONS=Image.open(A/'RunicWheelIcons-PREVIEW.png').convert('RGBA')
FRAME=Image.open(A/'RunicWheelFrame-PREVIEW.png').convert('RGBA')
PLAQUE=Image.open(A/'RunicWheelPlaque-PREVIEW.png').convert('RGBA')
FONT='/usr/share/fonts/truetype/dejavu/DejaVuSerif.ttf'
# Angle/labels chosen as test fixtures; every icon belongs to its actual corresponding type.
fixtures=[(0,'ÉPÉE EN ÉBÈNE'),(2,'HACHE DE FER'),(9,'BOUCLIER'),(17,'FLAMMES'),
          (21,'POTION'),(23,'NOURRITURE'),(11,'CASQUE'),(6,'ARC D’ÉBÈNE')]
for W,H,bgname in [(1280,720,'dark'),(1920,1080,'dark'),(1920,1080,'light'),(2560,1440,'dark')]:
    # no false game scene; neutral gradients represent challenging dark/bright backdrops
    y=np.linspace(0,1,H,dtype=np.float32)[:,None,None]
    x=np.linspace(0,1,W,dtype=np.float32)[None,:,None]
    base=np.array([28,39,52] if bgname=='dark' else [156,163,164],dtype=np.float32)
    grads=np.array([22,14,8] if bgname=='dark' else [18,14,11],dtype=np.float32)
    img=np.empty((H,W,4),np.uint8)
    img[:,:,:3]=np.clip(base+(y*.75+x*.25)*grads,0,255).astype('uint8')
    img[:,:,3]=255
    bg=Image.fromarray(img,'RGBA');d=ImageDraw.Draw(bg,'RGBA')
    cx=W/2;cy=H/2-min(H*.045,40);outer=min(min(W,H)*.31*1.30,440,H*.44)
    inner=outer*.36
    # draw sector opacity matching runtime; explicit full angular polygons
    for sector in range(8):
        t0=math.radians(-112.5+sector*45+.45)
        t1=math.radians(-112.5+(sector+1)*45-.45)
        shape=[]
        for k in range(26):
            a=t0+(t1-t0)*k/25;shape.append((cx+outer*.995*math.cos(a),cy+outer*.995*math.sin(a)))
        for k in range(26):
            a=t1-(t1-t0)*k/25;shape.append((cx+inner*math.cos(a),cy+inner*math.sin(a)))
        d.polygon(shape,fill=(139,108,63,179) if sector==0 else (24,32,43,124))
    n=round(outer*2)
    bg.alpha_composite(FRAME.resize((n,n),Image.Resampling.LANCZOS),(round(cx-outer),round(cy-outer)))
    # Inset gilded sweep for selected segment (focus sector 0).
    d.arc((cx-outer*.872,cy-outer*.872,cx+outer*.872,cy+outer*.872),-112.5,-67.5,
          fill=(255,219,152,220),width=max(2,round(outer*.012)))
    for sector,(id,name) in enumerate(fixtures):
        a=math.radians(-90+sector*45)
        ix=cx+math.cos(a)*outer*.66;iy=cy+math.sin(a)*outer*.66
        tile=ICONS.crop(((id%8)*256,(id//8)*256,(id%8+1)*256,(id//8+1)*256))
        size=outer*.398
        if size>181:size=181
        if sector==0:size*=1.055
        size=int(round(size))
        tile=tile.resize((size,size),Image.Resampling.LANCZOS)
        bg.alpha_composite(tile,(round(ix-size/2),round(iy-size/2)))
        # ShowSectorLabels defaults to 0: elegant uncluttered segments,
        # selected item and its category are unambiguously named on the plaque.
    y=cy+outer+10
    plateW=outer*1.18;plateH=80
    p=PLAQUE.resize((round(plateW),round(plateH)),Image.Resampling.LANCZOS)
    bg.alpha_composite(p,(round(cx-plateW/2),round(y-18)))
    for txt,fs,top,col in [('ÉPÉE EN ÉBÈNE',27,y-1,(254,240,212,255)),('WEAPON',14,y+28,(182,215,220,255))]:
        f=ImageFont.truetype(FONT,fs)
        d.text((cx,top),txt,anchor='mt',font=f,fill=col,stroke_width=1,stroke_fill=(7,12,18,240))
    path=A/f'V015-UI-{W}x{H}-{bgname}.png'
    bg.convert('RGB').save(path)
    if bgname=='dark':bg.convert('RGB').save(A/f'UI-PRO-PREVIEW-{W}x{H}.png')
    print(path)
