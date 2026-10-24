#!/usr/bin/env python3
from pathlib import Path
from PIL import Image, ImageDraw, ImageFilter
import math, struct, hashlib
ROOT=Path(__file__).resolve().parent.parent
ART=ROOT/'art'
base=Image.open(ART/'V017-Heraldry-Base.png').convert('RGBA')
out=base.copy()

CELL=256

def cell_box(i):
    return (i%8*CELL, i//8*CELL, i%8*CELL+CELL, i//8*CELL+CELL)

def get_tile(img,i):
    return img.crop(cell_box(i)).convert('RGBA')

def put_tile(img,i,tile):
    img.paste(tile, cell_box(i))

def glow_layer(size, color, rings):
    lay=Image.new('RGBA',(size,size),(0,0,0,0))
    d=ImageDraw.Draw(lay,'RGBA')
    c=size//2
    for r,a in rings:
        d.ellipse((c-r,c-r,c+r,c+r), outline=(*color,a), width=max(1,size//64))
    return lay.filter(ImageFilter.GaussianBlur(size//64))

def points_star(cx,cy,r1,r2,n,rot=-90):
    pts=[]
    for i in range(n*2):
        ang=math.radians(rot + i*180/n)
        r=r1 if i%2==0 else r2
        pts.append((cx+math.cos(ang)*r, cy+math.sin(ang)*r))
    return pts

def school_tile(base_idx,bg_tint,glow_col, painter):
    tile=get_tile(base,base_idx)
    # Mute the original center but preserve the forged rim and outer engraving.
    wash=Image.new('RGBA',(CELL,CELL),(0,0,0,0))
    wd=ImageDraw.Draw(wash,'RGBA')
    wd.ellipse((38,38,218,218), fill=(*bg_tint,185))
    wd.ellipse((48,48,208,208), outline=(255,255,255,18), width=2)
    tile.alpha_composite(wash)
    tile.alpha_composite(glow_layer(CELL,glow_col,[(80,66),(69,58),(58,42)]))

    S=512
    sym=Image.new('RGBA',(S,S),(0,0,0,0))
    ds=ImageDraw.Draw(sym,'RGBA')
    painter(ds,S)
    glow=sym.filter(ImageFilter.GaussianBlur(10))
    tile.alpha_composite(glow.resize((CELL,CELL),Image.Resampling.LANCZOS))
    tile.alpha_composite(sym.resize((CELL,CELL),Image.Resampling.LANCZOS))
    return tile

def paint_alteration(d,S):
    c=S/2
    hex_pts=[(c+math.cos(math.radians(60*i-30))*118,c+math.sin(math.radians(60*i-30))*118) for i in range(6)]
    d.polygon(hex_pts, outline=(228,214,164,255), width=16)
    d.line((c,c-126,c,c+126), fill=(150,233,255,230), width=18)
    d.line((c-70,c-18,c,c-126,c+70,c-18,c,c+126,c-70,c-18), fill=(188,244,255,230), width=14)
    d.polygon([(c,c-104),(c+56,c-8),(c,c+96),(c-56,c-8)], outline=(200,245,255,255), fill=(92,190,255,76))
    d.arc((c-88,c-88,c+88,c+88), 18, 162, fill=(255,255,255,180), width=12)
    d.arc((c-88,c-88,c+88,c+88), 198, 342, fill=(255,255,255,180), width=12)

def paint_conjuration(d,S):
    c=S/2
    d.ellipse((c-126,c-126,c+126,c+126), outline=(190,155,255,245), width=14)
    tri=[(c,c-118),(c+108,c+76),(c-108,c+76)]
    d.polygon(tri, outline=(233,216,168,240), width=14)
    d.line((tri[0],tri[1],tri[2],tri[0]), fill=(170,220,255,110), width=4)
    for ang in (0,72,144,216,288):
        a=math.radians(ang-90)
        x,y=c+math.cos(a)*92,c+math.sin(a)*92
        d.ellipse((x-11,y-11,x+11,y+11), fill=(215,245,255,235), outline=(255,255,255,180), width=4)
    d.arc((c-58,c-58,c+58,c+58), 0, 360, fill=(124,224,255,220), width=10)

def paint_destruction(d,S):
    c=S/2
    flame=[(c,c-120),(c+46,c-46),(c+24,-999)]
    flame=[(c,c-124),(c+56,c-40),(c+28,c-42),(c+80,c+34),(c+32,c+28),(c+18,c+120),(c-12,c+68),(c-56,c+124),(c-42,c+42),(c-98,c+38),(c-52,c-22),(c-76,c-24)]
    d.polygon(flame, fill=(255,132,36,180), outline=(255,220,162,255))
    crack=[(c-34,c-120),(c+10,c-18),(c-10,c-18),(c+42,c+110)]
    d.line(crack, fill=(255,246,208,255), width=18)
    d.line((c-106,c+46,c-18,c+10,c+76,c+40), fill=(255,216,180,170), width=10)

def paint_illusion(d,S):
    c=S/2
    d.arc((c-150,c-86,c+150,c+86), 200, 340, fill=(220,202,255,245), width=16)
    d.arc((c-150,c-86,c+150,c+86), 20, 160, fill=(220,202,255,245), width=16)
    d.ellipse((c-42,c-42,c+42,c+42), fill=(166,222,255,185), outline=(248,246,255,255), width=10)
    d.ellipse((c-14,c-14,c+14,c+14), fill=(255,255,255,230))
    d.arc((c-120,c-130,c+10,c), 120, 282, fill=(158,255,244,210), width=14)
    for x in (-82,0,82):
        d.arc((c+x-28,c+54,c+x+28,c+110), 180, 360, fill=(158,255,244,150), width=10)

def paint_restoration(d,S):
    c=S/2
    for i in range(12):
        a=math.radians(i*30)
        r1,r2=42,132
        d.line((c+math.cos(a)*r1,c+math.sin(a)*r1,c+math.cos(a)*r2,c+math.sin(a)*r2), fill=(255,228,146,225), width=10)
    d.ellipse((c-72,c-72,c+72,c+72), fill=(255,216,86,76), outline=(255,240,198,240), width=14)
    d.line((c,c-70,c,c+70), fill=(255,247,230,255), width=18)
    d.line((c-54,c,c+54,c), fill=(255,247,230,255), width=18)
    d.ellipse((c-18,c-18,c+18,c+18), fill=(255,252,240,255))

def paint_power(d,S):
    c=S/2
    d.polygon(points_star(c,c,132,66,8), fill=(168,34,54,92), outline=(235,188,116,255))
    d.ellipse((c-112,c-112,c+112,c+112), outline=(255,224,180,230), width=12)
    d.ellipse((c-54,c-54,c+54,c+54), fill=(208,38,56,148), outline=(255,243,210,240), width=12)
    # stylised claw/dragon mark
    claw=[(c-36,c+76),(c-28,c-68),(c-2,c-32),(c+4,c-110),(c+34,c-36),(c+50,c-86),(c+70,c+36)]
    d.line(claw, fill=(255,245,215,245), width=16)
    d.line((c-76,c+24,c+76,c-28), fill=(255,207,126,140), width=10)

def paint_shout_overlay(tile):
    ov=Image.new('RGBA',(CELL,CELL),(0,0,0,0))
    d=ImageDraw.Draw(ov,'RGBA')
    for offs,a in ((0,170),(14,120),(28,82)):
        d.arc((86-offs,82-offs,214+offs,174+offs), 310, 50, fill=(220,247,255,a), width=5)
    tile.alpha_composite(ov)
    return tile

# Schools and power occupy indices 16..21 in v0.18.
customs={
    16:(18,(32,54,76),(116,214,255),paint_alteration),
    17:(16,(54,34,76),(196,134,255),paint_conjuration),
    18:(17,(82,36,16),(255,132,36),paint_destruction),
    19:(31,(40,38,68),(170,245,240),paint_illusion),
    20:(20,(72,60,26),(255,214,96),paint_restoration),
    21:(31,(76,26,26),(255,146,96),paint_power),
}
for idx,(base_idx,bg,glow,painter) in customs.items():
    put_tile(out,idx,school_tile(base_idx,bg,glow,painter))
# Re-index the remaining utility icons to match the v0.18 enum layout.
remap={22:21,23:22,24:23,25:24,27:26,28:27,29:28,30:29,31:31}
for target,source in remap.items():
    put_tile(out,target,get_tile(base,source))
put_tile(out,26,paint_shout_overlay(get_tile(base,25)))

(out).save(ART/'V018-Arcana-Atlas.png')
out.save(ART/'Icons-PRO-PREVIEW.png')
out.save(ART/'RunicWheelIcons-PREVIEW.png')
raw=b'RWAT'+struct.pack('<II',2048,1024)+Image.Image.tobytes(out.convert('RGBA'),'raw','BGRA')
(ROOT/'RunicWheelIcons.rwtx').write_bytes(raw)
print('SHA256', hashlib.sha256(raw).hexdigest())
