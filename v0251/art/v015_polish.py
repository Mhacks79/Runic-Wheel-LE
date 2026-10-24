#!/usr/bin/env python3
"""Runic Wheel LE V0.15: reproducible original icon medallions + Nordic frame + plaque.
The glyph paths are our independently authored v0.14 vectors, not Wheeler/Skyrim assets.
All exported textures are raw 32-bit BGRA RWAT used by D3D9. No font files in delivery.
"""
import importlib.util, math, random, hashlib, struct
from io import BytesIO
from pathlib import Path
import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageFont
import cairosvg
P=Path(__file__).resolve().parent.parent
ART=P/'art'; ART.mkdir(exist_ok=True)
spec=importlib.util.spec_from_file_location('original_art',ART/'generate.py')
prior=importlib.util.module_from_spec(spec);spec.loader.exec_module(prior)
ICONS=prior.ICONS
S=256
rnd=random.Random(155015)

def write(name,image):
    img=image.convert('RGBA');w,h=img.size
    rgba=np.asarray(img)
    path=P/name
    path.write_bytes(b'RWAT'+struct.pack('<II',w,h)+rgba[:,:,[2,1,0,3]].tobytes())
    img.save(ART/(name.replace('.rwtx','-PREVIEW.png')))
    print(name,w,h,hashlib.sha256(path.read_bytes()).hexdigest()[:16]);return image

def line(d,coords,color,width=1):d.line(coords,fill=color,width=width,joint='curve')
def pnt(x,y,r,deg):
    a=math.radians(deg);return (x+r*math.cos(a),y+r*math.sin(a))
def ring(d,cx,cy,r,color,width):d.ellipse((cx-r,cy-r,cx+r,cy+r),outline=color,width=width)

def medallion(name,basecolor):
    from PIL import ImageColor
    color=ImageColor.getrgb(basecolor)
    yy,xx=np.ogrid[:S,:S];cx=cy=127.5;R=np.sqrt((xx-cx)**2+(yy-cy)**2)
    ang=np.arctan2(yy-cy,xx-cx)
    rng=np.random.default_rng(int(hashlib.sha256(name.encode()).hexdigest()[:8],16))
    # Microsurface shading: enamel interior, concentric ground-metal bevel, highlighted upper-left rim.
    lighting=np.clip(.46+.19*np.sin(ang+.9)+.26*np.cos(ang+2.1),.2,1)
    grain=rng.normal(0,2.7,(S,S))
    img=np.zeros((S,S,4),dtype=np.uint8)
    opaque=R<113
    enamel=(R<98)
    metallic=(R>=97)&opaque
    for k in range(3):
        ch=([20,28,34][k]+color[k]*.16+lighting*28+grain).clip(0,255)
        brass=([90,73,50][k]+color[k]*.22+lighting*77+grain*1.25).clip(0,255)
        img[:,:,k]=np.where(enamel,ch,np.where(metallic,brass,0)).astype(np.uint8)
    # Transparent outside circle; high transparency is deliberately retained at the edge.
    img[:,:,3]=np.where(opaque,244,0).astype('uint8')
    tile=Image.fromarray(img,'RGBA')
    d=ImageDraw.Draw(tile,'RGBA')
    for r,co,w in [(112,(8,12,17,250),4),(108,(231,203,154,240),2),(105,(70,81,87,250),2),(102,(240,213,163,145),1),
                   (99,(12,17,20,240),3),(94,(105,125,135,195),1),(89,(187,161,122,115),1)]:ring(d,128,128,r,co,w)
    # Inner etched concentric ripples and Celtic-type knotwork, not a borrowed logo.
    for angle in range(0,360,15):
        rad1=101 if angle%45 else 96
        a=pnt(128,128,rad1,angle);b=pnt(128,128,105,angle)
        line(d,[a,b],(239,214,163,186),2 if angle%45 else 3)
    for angle in (0,90,180,270):
        x,y=pnt(128,128,110,angle)
        d.polygon([(x,y-6),(x+6,y),(x,y+6),(x-6,y)],fill=(31,38,40,250),outline=(236,205,149,234))
    for a in range(8):
        th=math.radians(a*45+22.5)
        a1=(128+math.cos(th)*78,128+math.sin(th)*78)
        b1=(128+math.cos(th+.052)*87,128+math.sin(th+.052)*87)
        c1=(128+math.cos(th-.055)*87,128+math.sin(th-.055)*87)
        line(d,[b1,a1,c1],(186,159,126,105),2)
    # Original vector symbol: independent path per type; shadow, colored aura and metal-shaded art.
    svg=f'''<svg xmlns="http://www.w3.org/2000/svg" width="184" height="184" viewBox="0 0 184 184"><defs>
    <linearGradient id="steel" x1=".08" y1="0" x2=".98" y2=".95"><stop stop-color="#fffdf1"/><stop offset=".27" stop-color="#ccdce4"/><stop offset=".53" stop-color="#6b8797"/><stop offset=".72" stop-color="#d0e1e2"/><stop offset="1" stop-color="#536976"/></linearGradient>
    <linearGradient id="bronze" x1="0" y1="0" x2=".94" y2=".9"><stop stop-color="#fceac2"/><stop offset=".40" stop-color="#c99b5e"/><stop offset=".75" stop-color="#755333"/><stop offset="1" stop-color="#f2cb86"/></linearGradient></defs>
    <g transform="translate(92 92) scale(1.00)">{prior.glyph_body(name)}</g></svg>'''
    glyph=Image.open(BytesIO(cairosvg.svg2png(bytestring=svg.encode(),output_width=184,output_height=184))).convert('RGBA')
    # Dark edge behind bright silhouette then minimal accent halo, no muddy glow.
    alpha=glyph.getchannel('A')
    shadow=Image.new('RGBA',glyph.size,(2,8,14,0));shadow.putalpha(alpha.filter(ImageFilter.MaxFilter(5)).filter(ImageFilter.GaussianBlur(3)).point(lambda n:int(n*.75)))
    halo=Image.new('RGBA',glyph.size,color+(0,));halo.putalpha(alpha.filter(ImageFilter.GaussianBlur(8)).point(lambda n:int(n*.31)))
    tile.alpha_composite(shadow,(37,41));tile.alpha_composite(halo,(35,35));tile.alpha_composite(glyph,(36,33))
    return tile

def make_icons():
    a=Image.new('RGBA',(2048,1024))
    for i,(name,color) in enumerate(ICONS):a.alpha_composite(medallion(name,color),((i%8)*256,(i//8)*256))
    out=write('RunicWheelIcons.rwtx',a)
    a.save(ART/'Icons-PRO-PREVIEW.png')
    return out

def frame():
    n=1024;cy=cx=512
    Y,X=np.ogrid[:n,:n];rr=np.sqrt((X-cx)**2+(Y-cy)**2)
    phi=np.arctan2(Y-cy,X-cx)
    lighting=.57+.20*np.cos(phi+2.25)
    rng=np.random.default_rng(1516)
    grain=rng.normal(0,2.9,(n,n))
    # Metallic concentric body, translucent inside. Do NOT cover sectors with opaque polygons.
    mask=(rr>=452)&(rr<496)
    rgb=np.zeros((n,n,4),dtype=np.uint8)
    gold=(rr>=474)&(rr<486)
    for k in range(3):
        metal=([21,28,34][k]+lighting*51+grain).clip(0,255)
        gilded=([96,73,48][k]+lighting*91+grain).clip(0,255)
        rgb[:,:,k]=np.where(mask,np.where(gold,gilded,metal),0).astype('uint8')
    rgb[:,:,3]=np.where(mask,233,0).astype('uint8')
    f=Image.fromarray(rgb,'RGBA');d=ImageDraw.Draw(f,'RGBA')
    for rad,c,w in [(495,(6,9,13,252),7),(492,(240,214,164,244),2),(488,(85,100,104,236),2),(485,(18,25,33,220),2),
                    (475,(227,183,119,215),3),(471,(65,76,82,240),3),(463,(242,208,147,190),2),(455,(16,21,27,247),4),
                    (449,(181,157,116,178),2),(426,(210,182,139,88),2)]:ring(d,cx,cy,rad,c,w)
    # 80 low-key engraved radial runes, individually drawn (not random ASCII text).
    for i in range(80):
        t=(i*360/80-90)
        r0=465+(2 if i%3 else 0);r1=469+(2 if i%3 else 0)
        line(d,[pnt(cx,cy,r0,t),pnt(cx,cy,r1,t)],(234,214,180,120),1)
        if i%4==0:
            A=pnt(cx,cy,459,t);B=pnt(cx,cy,468,t-1.1);C=pnt(cx,cy,468,t+1.1)
            line(d,[B,A,C],(222,190,141,148),2)
    # Engraved wedge dividers with second thin raised edge and anchored rhombus rune.
    for i in range(8):
        t=-112.5+45*i
        line(d,[pnt(cx,cy,175,t),pnt(cx,cy,452,t)],(8,13,19,232),8)
        line(d,[pnt(cx,cy,182,t),pnt(cx,cy,449,t)],(199,174,132,165),3)
        line(d,[pnt(cx,cy,190,t+.5),pnt(cx,cy,443,t+.5)],(231,224,204,74),1)
        x,y=pnt(cx,cy,458,t)
        xy=[(x,y-12),(x+10,y),(x,y+12),(x-10,y)]
        d.polygon(xy,fill=(27,34,40,245),outline=(241,209,147,245))
        d.polygon([(x,y-5),(x+4,y),(x,y+5),(x-4,y)],fill=(101,156,172,225))
    # Center medallion: multi-depth dark iron ring, delicate interlocked eightpoint original sigil.
    d.ellipse((337,337,687,687),fill=(13,22,31,243),outline=(15,21,27,251),width=10)
    for r,w,c in [(171,5,(220,186,131,235)),(165,2,(249,233,193,166)),(159,5,(50,65,73,248)),(152,2,(197,166,114,200)),
                  (143,1,(110,138,148,140)),(118,3,(123,148,149,150)),(104,2,(206,169,113,143))]:ring(d,cx,cy,r,c,w)
    # radial stars and tangled compass lines
    for i in range(16):
        t=-90+i*22.5
        r0=126 if i%2 else 116;r1=143 if i%2 else 146
        line(d,[pnt(cx,cy,r0,t),pnt(cx,cy,r1,t)],(185,152,105,138),1 if i%2 else 2)
    pts=[]
    for i in range(8):pts.append(pnt(cx,cy,96,-90+i*45))
    for j in range(8):line(d,[pts[j],pts[(j+3)%8]],(156,188,196,95),2)
    # original heraldic compass crystal with layered metallic ridges
    diamond=[(512,411),(579,512),(512,613),(445,512)]
    d.polygon(diamond,fill=(25,39,51,240),outline=(237,202,143,236),width=5)
    d.polygon([(512,429),(558,512),(512,595),(466,512)],fill=(30,49,62,225),outline=(112,170,185,225),width=3)
    d.polygon([(512,445),(540,512),(512,575)],fill=(95,136,153,210))
    line(d,[(512,421),(512,605)],(243,218,170,231),2)
    # four ornate cardinal diamonds, no copied dragon sigils.
    for t in [-90,0,90,180]:
        x,y=pnt(cx,cy,491,t)
        d.polygon([(x,y-15),(x+10,y),(x,y+15),(x-10,y)],fill=(24,35,43,240),outline=(239,205,150,248))
        d.polygon([(x,y-5),(x+4,y),(x,y+5),(x-4,y)],fill=(159,197,203,232))
    return write('RunicWheelFrame.rwtx',f)

def plaque():
    n=1024;h=256
    yy,xx=np.ogrid[:h,:n]
    rng=np.random.default_rng(90915)
    noise=rng.normal(0,2.7,(h,n))
    bg=Image.new('RGBA',(n,h),(0,0,0,0))
    # richly layered trapezoid but transparent outside; corner swept knotwork.
    d=ImageDraw.Draw(bg,'RGBA')
    outer=[(79,19),(945,19),(997,124),(945,237),(79,237),(27,124)]
    d.polygon(outer,fill=(17,24,30,234),outline=(205,167,113,240),width=6)
    d.line([(82,28),(940,28),(984,124),(940,228),(82,228),(40,124),(82,28)],fill=(69,101,112,190),width=3)
    for y in [53,215]:d.line([(113,y),(911,y)],fill=(174,146,99,75),width=2)
    d.line([(154,158),(870,158)],fill=(204,176,123,173),width=2)
    for flip in [False,True]:
        f=lambda x:(n-x) if flip else x
        for points in [[(78,49),(127,121),(77,210)],[(93,55),(151,121),(92,204)],[(56,124),(80,102),(105,124),(80,146),(56,124)]]:
            d.line([(f(x),y) for x,y in points],fill=(226,190,134,185),width=3)
        d.ellipse((f(100)-6,118,f(100)+6,130),fill=(108,173,193,230),outline=(231,208,158,235),width=2)
    for x in [203,821]:
        d.polygon([(x,118),(x+12,129),(x,139),(x-12,129)],fill=(27,48,55,235),outline=(237,203,151,220))
    return write('RunicWheelPlaque.rwtx',bg)

if __name__=='__main__':
    make_icons();frame();plaque();prior.font_atlas()
    import subprocess,sys
    subprocess.check_call([sys.executable,str(ART/"apply_approved_center.py")])
