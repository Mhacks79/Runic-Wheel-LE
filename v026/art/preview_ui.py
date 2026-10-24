from pathlib import Path
from PIL import Image,ImageDraw,ImageFilter,ImageFont
import math
P=Path(__file__).resolve().parents[1];A=P/'art'
ICONS=Image.open(A/'Icons-PRO-PREVIEW.png').convert('RGBA')
FRAME=Image.open(A/'Frame-PRO-PREVIEW.png').convert('RGBA')
F='/usr/share/fonts/truetype/dejavu/DejaVuSerif.ttf'
scenes=[('1920x1080',1920,1080),('1280x720',1280,720),('2560x1440',2560,1440)]
labels=['ÉPÉE EN ÉBÈNE','BOUCLIER DE FER','SORT DE FEU','ARC D’ÉBÈNE','INGRÉDIENT','CUIRASSE','NOURRITURE','HACHE DE FER']
ids=[0,9,17,6,24,10,23,2]
for slug,W,H in scenes:
    bg=Image.new('RGB',(W,H),(26,32,42));pixels=bg.load()
    # Warm/cool low contrast game-like backdrop, intentionally not a screenshot claim.
    for y in range(H):
        for x in range(W):
            t=y/H
            pixels[x,y]=(int(30+24*t+3*x/W),int(41+19*t),int(51+8*t))
    bg=bg.convert('RGBA')
    cx,cy=W*.5,H*.5
    outer=min(W,H)*.31*1.3;outer=min(outer,440)
    d=ImageDraw.Draw(bg,'RGBA')
    for i in range(8):
        a=-112.5+i*45;b=a+45;pp=[]
        for k in range(12):
            rad=math.radians(a+(b-a)*k/11)
            pp.append((cx+outer*.99*math.cos(rad),cy+outer*.99*math.sin(rad)))
        for k in range(12):
            rad=math.radians(b-(b-a)*k/11)
            pp.append((cx+outer*.36*math.cos(rad),cy+outer*.36*math.sin(rad)))
        d.polygon(pp,fill=((99,79,50,160) if i==0 else (31,44,57,112)))
    fr=FRAME.resize((int(outer*2),int(outer*2)),Image.Resampling.LANCZOS)
    bg.alpha_composite(fr,(round(cx-outer),round(cy-outer)))
    for sector in range(8):
        theta=math.radians(-90+sector*45)
        xx=cx+math.cos(theta)*outer*.66;yy=cy+math.sin(theta)*outer*.66-6
        id=ids[sector]
        tile=ICONS.crop(((id%8)*256,(id//8)*256,(id%8+1)*256,(id//8+1)*256))
        sz=min(outer*.355,172)
        if sector==0:sz*=1.035
        tile=tile.resize((round(sz),round(sz)),Image.Resampling.LANCZOS)
        bg.alpha_composite(tile,(round(xx-sz*.5),round(yy-sz*.5)))
        font=ImageFont.truetype(F,round(16*1.15))
        text=labels[sector]
        pos=(xx,yy+outer*.175)
        wd=d.textlength(text,font=font)
        if wd>150:
            while wd>142 and len(text)>2:
                text=text[:-2]+'…';wd=d.textlength(text,font=font)
        d.rounded_rectangle((xx-81,pos[1]-4,xx+81,pos[1]+22),4,fill=(12,21,29,195),outline=(151,119,79,90),width=1)
        d.text((xx-wd*.5,pos[1]-3),text,font=font,fill=(245,234,211,255),stroke_width=1,stroke_fill=(9,13,17,245))
    cyName=cy+outer+13
    cw=outer*.98
    d.rectangle((cx-cw*.5,cyName-7,cx+cw*.5,cyName+42),fill=(15,21,27,220))
    d.line((cx-cw*.5,cyName-6,cx+cw*.5,cyName-6),fill=(200,161,104,240),width=2)
    nf=ImageFont.truetype(F,25)
    n='ÉPÉE EN ÉBÈNE'
    tw=d.textlength(n,font=nf)
    d.text((cx-tw*.5,cyName-2),n,font=nf,fill=(254,242,220),stroke_width=1,stroke_fill=(0,0,0))
    outf=A/f'UI-PRO-PREVIEW-{slug}.png';bg.convert('RGB').save(outf)
    print(outf)
