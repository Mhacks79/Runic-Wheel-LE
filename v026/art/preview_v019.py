#!/usr/bin/env python3
"""Offline 1920x1080 / 1280x720 visualization, not an in-game screenshot."""
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
import math
P=Path(__file__).resolve().parents[1]
A=P/'art'
ICONS=Image.open(A/'V018-Arcana-Atlas.png').convert('RGBA')
FRAME=Image.open(A/'Frame-PRO-PREVIEW.png').convert('RGBA')
F='/usr/share/fonts/truetype/dejavu/DejaVuSerif.ttf'
labels=['STEEL SWORD','ALTERATION','CONJURATION','DESTRUCTION','ILLUSION','RESTORATION','POWER','SHOUT']
ids=[0,16,17,18,19,20,21,26]
for W,H in ((1920,1080),(1280,720)):
    bg=Image.new('RGB',(W,H),(16,24,33))
    pixels=bg.load()
    for y in range(H):
        for x in range(W):
            t=y/H
            pixels[x,y]=(int(18+12*t+3*x/W),int(26+13*t),int(34+11*t))
    bg=bg.convert('RGBA'); d=ImageDraw.Draw(bg,'RGBA')
    outer=min(min(W,H)*.31*1.3,440.,H*.44)
    cx=W*.5; cy=H*.5-min(H*.045,40)
    for s in range(8):
        pp=[]
        for k in range(14):
            a=math.radians(-112.5+s*45+45*k/13)
            pp.append((cx+outer*.98*math.cos(a),cy+outer*.98*math.sin(a)))
        for k in range(14):
            a=math.radians(-67.5+s*45-45*k/13)
            pp.append((cx+outer*.36*math.cos(a),cy+outer*.36*math.sin(a)))
        d.polygon(pp,fill=(107,78,45,148) if s==0 else (29,40,54,155))
    fr=FRAME.resize((round(outer*2),round(outer*2)),Image.Resampling.LANCZOS)
    bg.alpha_composite(fr,(round(cx-outer),round(cy-outer)))
    fs=max(12,round(H/60))
    font=ImageFont.truetype(F,fs)
    for s in range(8):
        th=math.radians(-90+s*45)
        xx=cx+math.cos(th)*outer*.66; yy=cy+math.sin(th)*outer*.66
        i=ids[s]
        cell=ICONS.crop((i%8*256,i//8*256,i%8*256+256,i//8*256+256))
        sz=round(min(outer*.355,165))
        bg.alpha_composite(cell.resize((sz,sz),Image.Resampling.LANCZOS),(round(xx-sz/2),round(yy-sz/2)))
        # show the first sector with a realistic 2/3 stacked-item counter
        if s==0:
            bf=ImageFont.truetype(F,max(12,round(fs*.81)))
            d.rounded_rectangle((xx+21,yy-sz*.51,xx+72,yy-sz*.51+26),radius=4,fill=(14,25,36,235),outline=(217,175,102,190),width=1)
            d.text((xx+28,yy-sz*.51+3),'2/3',fill=(246,212,148,255),font=bf)
        text=labels[s];tw=d.textlength(text,font=font)
        y=yy+outer*.20
        d.rounded_rectangle((xx-max(64,tw/2+9),y-5,xx+max(64,tw/2+9),y+fs+8),radius=3,fill=(9,18,25,229),outline=(165,131,85,140),width=1)
        d.text((xx-tw/2,y-1),text,fill=(246,231,208,255),font=font)
    # customisable page title (external)
    titleY=max(18,cy-outer-31)
    d.rounded_rectangle((cx-112,titleY,cx+112,titleY+27),radius=4,fill=(10,20,29,238),outline=(206,169,101,190),width=1)
    titleFont=ImageFont.truetype(F,max(14,round(fs*1.05)))
    title='ARCANA';tw=d.textlength(title,font=titleFont)
    d.text((cx-tw/2,titleY+3),title,fill=(248,222,174,255),font=titleFont)
    # bottom plaque with stack context
    y=cy+outer+11
    w=min(outer*1.20,515)
    d.rounded_rectangle((cx-w/2,y-12,cx+w/2,y+53),radius=6,fill=(10,19,27,236),outline=(182,150,97,210),width=2)
    nameFont=ImageFont.truetype(F,max(17,round(fs*1.26)))
    smaller=ImageFont.truetype(F,max(12,round(fs*.84)))
    for text,ft,yy,color in [('STEEL SWORD',nameFont,y-6,(252,240,220,255)),('WEAPON  [2/3]',smaller,y+27,(190,218,216,255))]:
        tw=d.textlength(text,font=ft)
        d.text((cx-tw/2,yy),text,font=ft,fill=color)
    dest=A/f'V019-UI-{W}x{H}-dark.png'
    bg.convert('RGB').save(dest,optimize=True)
    print(dest)
