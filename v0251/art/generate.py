#!/usr/bin/env python3
"""Deterministic, individually authored Runic Wheel LE art pipeline.
Assets are self-contained BGRA textures for a fixed-function D3D9 renderer.
No cropped image-sheet icons, no copyrighted Skyrim/Wheeler resources.
"""
from pathlib import Path
import math, struct, hashlib, sys
import numpy as np
from PIL import Image, ImageDraw, ImageFont, ImageFilter
import cairosvg
from io import BytesIO
ROOT=Path(__file__).resolve().parents[1]
ART=ROOT/'art'
ART.mkdir(exist_ok=True)
S=256
# index is public binary/enum contract. 8 columns * 4 rows, exactly one sprite per tile.
ICONS=[
 ('sword','#d9ae6b'),('dagger','#d9ae6b'),('axe','#d9ae6b'),('mace','#d9ae6b'),
 ('greatsword','#e3b569'),('warhammer','#e3b569'),('bow','#a6c488'),('crossbow','#a6c488'),
 ('staff','#b9a0e9'),('shield','#b9ccd8'),('armor','#b9ccd8'),('helmet','#b9ccd8'),
 ('gauntlet','#b9ccd8'),('boots','#b9ccd8'),('ring','#ddc68b'),('amulet','#ddc68b'),
 ('magic','#ba94ec'),('fire','#f5a56b'),('frost','#91cee7'),('lightning','#e7d17b'),
 ('healing','#ebd7a0'),('potion','#e57979'),('poison','#76bf85'),('food','#e0b77e'),
 ('ingredient','#83bf95'),('shout','#8dc6e6'),('arrows','#d9c999'),('bolts','#d9c999'),
 ('torch','#e6ae6b'),('misc','#b7c3cd'),('empty','#768896'),('scroll','#ceba9d'),
]
assert len(ICONS)==32

def glyph_body(n):
    """Original scalable icon symbol in viewBox -70..70. No third party path library."""
    b='#eef1e9';e='#b88752';d='#243746';a='#ffffff'
    # use layered path construction for a strong silhouette, subtle engraved details
    sword='''<path d="M0 -68 L12 -37 L7 29 L0 39 L-7 29 L-12 -37Z" fill="url(#steel)" stroke="#eaf4fa" stroke-width="2"/><path d="M0 -58 V31" stroke="#496576" stroke-width="2"/><path d="M-30 31 Q-10 37 0 29 Q10 37 30 31 L27 43 Q9 40 5 41 V61 H-5 V41 Q-9 40-27 43 Z" fill="url(#bronze)" stroke="#f4dc9f" stroke-width="2"/><circle cx="0" cy="62" r="6" fill="#deb878" stroke="#fff2ce" stroke-width="1.8"/>'''
    dagger='''<path d="M0 -53 L14 -24 L4 16 L0 25 L-4 16 L-14 -24Z" fill="url(#steel)" stroke="#e4edf1" stroke-width="2"/><path d="M0 -47 V21" stroke="#486276" stroke-width="2"/><path d="M-24 21 Q-11 27 0 20 Q11 27 24 21 L19 34 H4 V55 H-4 V34 H-19Z" fill="url(#bronze)" stroke="#f9d7a1" stroke-width="2"/><path d="M-3 55 L0 65 L3 55" fill="#e6cc91"/>'''
    axe='''<path d="M-7 -57 L5 -59 L18 63 L7 65 Z" fill="url(#bronze)" stroke="#e7c594" stroke-width="2"/><path d="M0 -47 Q43 -57 56 -27 Q36 -17 51 9 Q13 14-3 -3Z" fill="url(#steel)" stroke="#e6f0ec" stroke-width="3"/><path d="M23 -34 Q38 -31 45 -25 Q25 -18 33 1" fill="none" stroke="#688895" stroke-width="3"/>'''
    mace='''<path d="M-5 -20 H5 V62 H-5Z" fill="url(#bronze)" stroke="#d5b37a" stroke-width="2"/><path d="M0 -58 L14 -43 L22 -24 L12 -11 H-12 L-22 -24 L-14 -43Z" fill="url(#steel)" stroke="#edf3ed" stroke-width="2"/><path d="M0 -67 L6 -53 H-6Z M-28 -47 L-14 -42 -21 -31Z M28 -47 L14 -42 21 -31Z M-30 -20 L-17 -25 -20 -10Z M30 -20 L17 -25 20 -10Z" fill="#d4e5ed" stroke="#5d7787" stroke-width="1"/><path d="M0 -50 V-12 M-17 -29 H17" stroke="#77929e" stroke-width="2"/>'''
    greatsword='''<g transform="scale(1.06)">'''+sword+'''</g><path d="M-27 53 H27" stroke="#eed49a" stroke-width="4"/>'''
    warhammer='''<path d="M-5 -51 H6 L8 62 H-7 Z" fill="url(#bronze)" stroke="#e8c891" stroke-width="2"/><path d="M-53 -51 L-32 -62 H36 L53 -48 L36 -22 H-34Z" fill="url(#steel)" stroke="#f0f4e9" stroke-width="3"/><path d="M-35 -55 V-30 M34 -55 V-30 M-25 -41 H25" stroke="#7895a2" stroke-width="3"/>'''
    bow='''<path d="M-20 -60 Q52 0 -20 60" fill="none" stroke="#d8b782" stroke-width="7" stroke-linecap="round"/><path d="M-20 -60 L-20 60" stroke="#deebdb" stroke-width="2"/><path d="M-59 0 H52 M36 -7 L52 0 L36 7" stroke="#e7d5b0" stroke-width="3" fill="none"/><path d="M-59 0 L-45 -4 L-45 4Z" fill="#e2ebdb"/><path d="M-20 -58 L-15 -54 M-20 58 L-15 54" stroke="#e9d3a0" stroke-width="4"/>'''
    crossbow='''<path d="M-9 -48 H10 L15 58 H-14Z" fill="url(#bronze)" stroke="#ecd39a" stroke-width="2"/><path d="M-60 -33 Q-15 -60 0 -34 Q15 -60 60 -33" fill="none" stroke="#d4b589" stroke-width="8"/><path d="M-59 -34 L59 -34" stroke="#e8eee8" stroke-width="2"/><path d="M0 -57 V50 M-6 -45 L0 -57 L6 -45" fill="none" stroke="#f7f3eb" stroke-width="3"/><circle cx="0" cy="-29" r="9" fill="#677b83" stroke="#e8cc95" stroke-width="3"/>'''
    staff='''<path d="M-5 -47 Q14 -2 -5 63" fill="none" stroke="#b98d60" stroke-width="10" stroke-linecap="round"/><path d="M-5 -47 Q14 -2 -5 63" fill="none" stroke="#e5c789" stroke-width="2"/><circle cx="0" cy="-45" r="23" fill="#425a80" stroke="#ddd6fe" stroke-width="4"/><path d="M0 -61 L12 -44 L0 -27 L-12 -44Z" fill="#b6a1f3" stroke="#f8eaff" stroke-width="2"/><path d="M-32 -56 L-15 -47 M15 -47 L33 -56" stroke="#d7c9f9" stroke-width="3"/>'''
    shield='''<path d="M0 -65 Q32 -53 53 -51 V-5 Q48 38 0 66 Q-48 38 -53 -5 V-51 Q-29 -55 0 -65Z" fill="url(#steel)" stroke="#efe3c7" stroke-width="4"/><path d="M0 -51 L36 -42 V-7 Q29 25 0 45 Q-30 23-36 -7 V-42Z" fill="#1d313c" stroke="#c1b08a" stroke-width="2"/><path d="M0 -41 L10 -9 L34 -8 L14 6 L20 29 L0 16 L-20 29 L-14 6 L-34 -8 L-10 -9Z" fill="#cbb78d" stroke="#f9e8ba" stroke-width="1.5"/>'''
    armor='''<path d="M-31 -61 L-60 -41 L-49 -11 L-33 -19 L-31 60 L31 60 L33 -19 L49 -11 L60 -41 L31 -61 L14 -43 H-14Z" fill="url(#steel)" stroke="#e9ede7" stroke-width="3"/><path d="M-14 -44 L0 -29 L14 -44 M-30 -20 Q0 2 30 -20 M-30 22 Q0 37 30 22 M-31 52 H31" fill="none" stroke="#cfb486" stroke-width="3"/><path d="M0 -29 V51" stroke="#6c8793" stroke-width="3"/>'''
    helmet='''<path d="M-46 -6 Q-50 -59 0 -64 Q50 -59 46 -6 L37 37 L14 51 L11 23 H-11 L-14 51 L-37 37Z" fill="url(#steel)" stroke="#f0eae0" stroke-width="3"/><path d="M0 -63 V19 M-33 -1 H33 M-36 16 H-12 M12 16 H36" stroke="#d2b989" stroke-width="4"/><path d="M-12 14 L0 28 L12 14" fill="none" stroke="#f2e5bc" stroke-width="3"/>'''
    gauntlet='''<path d="M-40 -55 Q-30 -65 -23 -51 L-16 -6 L-6 -47 Q2 -57 9 -43 L7 -2 L18 -37 Q28 -44 33 -33 L21 7 L29 0 Q49 -10 50 7 Q34 35 17 55 H-26 Q-50 28-48 -2Z" fill="url(#steel)" stroke="#f1ead7" stroke-width="3"/><path d="M-26 37 H19 M-19 6 L0 20" stroke="#cdb58a" stroke-width="3"/>'''
    boots='''<path d="M-47 -57 H-6 V7 Q-10 22 10 27 L10 50 Q-4 60-50 52 Q-58 45-54 27Z M12 -53 H46 V4 Q42 20 57 30 V49 Q35 60 10 52Z" fill="url(#steel)" stroke="#f4efe3" stroke-width="3"/><path d="M-44 12 H-4 M14 10 H45 M-43 39 H5 M16 39 H53" stroke="#c8b68e" stroke-width="3"/>'''
    ring='''<ellipse cy="12" rx="34" ry="42" fill="none" stroke="url(#bronze)" stroke-width="17"/><path d="M0 -66 L18 -44 L0 -19 L-18 -44Z" fill="#78c4ce" stroke="#f8dc94" stroke-width="4"/><path d="M0 -61 V-24 M-12 -44 H12" stroke="#d5f6fc" stroke-width="2"/>'''
    amulet='''<path d="M-48 -53 Q0 -12 48 -53" fill="none" stroke="#d5c69a" stroke-width="6"/><circle cy="5" r="36" fill="#334957" stroke="#f5d6a1" stroke-width="7"/><path d="M0 -35 L16 5 L0 43 L-16 5Z" fill="#9bd0d5" stroke="#fff4dc" stroke-width="2"/>'''
    magic='''<path d="M-22 51 Q-51 22 -24 -5 Q-2 -20 1 -58 Q23 -28 14 -14 Q49 -33 53 -1 Q59 30 18 54Z" fill="#684a9e" stroke="#d6b5ff" stroke-width="4"/><path d="M-37 4 Q0 -14 35 6 M-25 27 Q13 7 28 34" fill="none" stroke="#edddff" stroke-width="3"/><circle cx="2" cy="-12" r="11" fill="#dbc2ff"/><path d="M-55 -40 L-47 -54 M48 -44 L57 -31" stroke="#ba9bff" stroke-width="4"/>'''
    fire='''<path d="M4 -67 Q32 -25 12 -6 Q42 -23 43 11 Q45 57 0 62 Q-49 49 -35 8 Q-15 -5-16 -33 Q1 -19 4 -67Z" fill="#cb551e" stroke="#ffd19b" stroke-width="3"/><path d="M4 -40 Q10 -12 0 4 Q24 -2 20 27 Q16 46 0 49 Q-23 44-19 21 Q-9 6 -7 -7 Q5 4 4 -40Z" fill="#ffcd6b" stroke="#fff0b5" stroke-width="2"/>'''
    frost='''<path d="M0 -65 V62 M-54 -31 L54 32 M-54 32 L54 -31 M-17 -49 L0 -31 L17 -49 M-18 48 L0 30 L18 48 M-52 -7 L-31 -8 L-43 -27 M51 6 L31 8 L44 28" fill="none" stroke="#c9f2ff" stroke-width="6" stroke-linecap="round"/><circle r="11" fill="#8edaff" stroke="#f4fbff" stroke-width="2"/>'''
    lightning='''<path d="M7 -67 L-32 5 L1 5 L-15 66 L45 -17 L10 -17Z" fill="#ffe18d" stroke="#fff8cf" stroke-width="3"/><path d="M7 -42 L-17 0 M17 -9 L1 34" stroke="#fef9c7" stroke-width="3"/>'''
    healing='''<circle r="39" fill="#415059" stroke="#e9d39a" stroke-width="4"/><path d="M-13 -63 H13 V-13 H63 V13 H13 V63 H-13 V13 H-63 V-13 H-13Z" fill="#f4deb2" stroke="#fff5e0" stroke-width="3"/><circle r="9" fill="#fff7e6"/>'''
    potion='''<path d="M-12 -55 H12 V-29 L29 -12 Q39 9 32 37 L24 54 H-24 L-32 37 Q-39 9 -29 -12 L-12 -29Z" fill="#8a3135" stroke="#f7c29a" stroke-width="4"/><path d="M-26 6 Q0 -10 26 6 L26 36 L20 47 H-20 L-26 35Z" fill="#db5d62"/><path d="M-18 -65 H18 V-50 H-18Z" fill="url(#bronze)" stroke="#f4d6a1" stroke-width="3"/><path d="M-13 12 Q-7 2 -2 7" stroke="#ffe6da" stroke-width="5"/>'''
    poison=potion.replace('#8a3135','#2a6e46').replace('#db5d62','#67aa66').replace('#f7c29a','#b7ebae')
    food='''<path d="M-52 -12 Q-27 -65 20 -41 Q54 -18 46 27 Q17 57 -31 40 Q-60 30 -52 -12Z" fill="#b77b47" stroke="#f7deb0" stroke-width="4"/><path d="M-38 -12 Q-4 -47 31 -14 M-24 16 Q4 -9 30 11" stroke="#e6b57b" stroke-width="8" stroke-linecap="round"/><path d="M23 35 Q50 13 57 33 Q55 49 37 54Z" fill="#edc77c" stroke="#fff0a5" stroke-width="2"/>'''
    ingredient='''<path d="M0 61 Q-7 18 4 -35" fill="none" stroke="#86b88c" stroke-width="6"/><path d="M-2 28 Q-41 2 -54 22 Q-35 43 -2 36 M0 8 Q36 -17 54 0 Q31 27 0 18" fill="#427d56" stroke="#b7dda1" stroke-width="2"/><circle cx="3" cy="-36" r="15" fill="#e4cbf3"/><ellipse cx="3" cy="-53" rx="13" ry="24" fill="#9c8cdb" stroke="#e3caff" stroke-width="2"/><ellipse cx="-18" cy="-41" rx="22" ry="12" transform="rotate(30 -18 -41)" fill="#9984d5" stroke="#ead2fc" stroke-width="2"/><ellipse cx="22" cy="-40" rx="22" ry="12" transform="rotate(-30 22 -40)" fill="#9984d5" stroke="#ead2fc" stroke-width="2"/><circle cx="3" cy="-35" r="8" fill="#f6da77"/>'''
    shout='''<path d="M-50 -18 Q-29 -62 5 -55 Q36 -53 51 -17 L38 -11 L58 7 L25 14 L20 38 L-12 49 L-45 32 L-56 0Z" fill="#43799b" stroke="#dbf0ff" stroke-width="4"/><path d="M-18 -18 Q5 -32 31 -17 M-16 10 L26 14 M-15 29 L10 34" stroke="#92d9f2" stroke-width="4" fill="none"/><circle cx="15" cy="-18" r="6" fill="#f2fafc"/><path d="M31 7 L51 17 L34 20" fill="#d4f5ff"/>'''
    arrows='''<path d="M-42 58 L28 -48 M-14 61 L53 -38 M-55 22 L16 -61" stroke="#e7d6a5" stroke-width="4"/><path d="M16 -61 L30 -50 L8 -46Z M28 -48 L41 -37 L19 -33Z M53 -38 L63 -28 L41 -22Z" fill="#dce8ea" stroke="#f8f9e5" stroke-width="2"/><path d="M-42 58 L-49 30 L-29 48 M-14 61 L-24 37 L-1 52" fill="none" stroke="#9bbaa8" stroke-width="4"/>'''
    bolts=arrows.replace('L28 -48','L22 -22').replace('L53 -38','L47 -18').replace('L16 -61','L13 -34').replace('16 -61','13 -34').replace('28 -48','22 -22').replace('53 -38','47 -18')
    torch='''<path d="M-5 -20 H6 L17 66 H-18Z" fill="url(#bronze)" stroke="#efd2a0" stroke-width="3"/><path d="M-26 -35 Q-17 -64 -2 -67 Q2 -40 13 -45 Q38 -22 19 7 Q-11 27-30 4 Q-43 -17-26 -35Z" fill="#e26d2f" stroke="#ffd393" stroke-width="3"/><path d="M-13 -22 Q-10 -41 -2 -48 Q4 -26 11 -16 Q18 8 0 13 Q-20 3-13 -22Z" fill="#ffe4a2"/>'''
    misc='''<path d="M-47 -45 H47 V46 H-47Z" fill="url(#steel)" stroke="#e4ddd0" stroke-width="4"/><path d="M-47 -16 H47 M-15 -45 V46 M14 -45 V46" stroke="#ccb585" stroke-width="3"/><circle cx="0" cy="1" r="13" fill="#9fc8c9" stroke="#e8f1df" stroke-width="3"/>'''
    empty='''<circle r="43" fill="none" stroke="#657480" stroke-width="4" stroke-dasharray="3 7"/><path d="M0 -30 L29 28 H-29Z" stroke="#88959d" stroke-width="6" fill="none"/><circle r="7" fill="#8fa1a8"/>'''
    scroll='''<path d="M-41 -51 H27 Q48 -51 48 -32 V47 H-32 Q-48 47 -48 30 V-33 Q-48 -51 -41 -51Z" fill="#cab591" stroke="#f6e7ce" stroke-width="4"/><path d="M-30 -28 H25 M-30 -11 H25 M-30 6 H11 M-30 22 H23" stroke="#7b6350" stroke-width="4"/><circle cx="36" cy="-38" r="11" fill="#a8765c" stroke="#e9c08c" stroke-width="3"/>'''
    return locals().get(n) or misc

def icon_sprite(name,color):
    from PIL import ImageColor
    rgb=ImageColor.getrgb(color)
    # 256x256 true independent tile, no neighbor sampling and >=12px transparent gutters.
    side=256; arr=np.zeros((side,side,4),dtype=np.uint8)
    yy,xx=np.ogrid[:side,:side]; r=np.sqrt((xx-127.5)**2+(yy-127.5)**2)
    mask=r<110
    glow=np.exp(-((r-106)/7)**2)*22
    shade=np.clip(.83-.0031*r+.18*np.cos((xx+yy)*.02),0,1)
    for c,ch in enumerate((27,34,41)):
        arr[:,:,c]=np.where(mask, np.uint8(np.clip(ch+rgb[c]*.12+shade*17+glow,0,255)),0)
    arr[:,:,3]=np.where(mask,225,0).astype('uint8')
    tile=Image.fromarray(arr,'RGBA')
    D=ImageDraw.Draw(tile,'RGBA')
    for rad,w,col in [(110,4,(167,142,108,220)),(104,2,(255,231,183,180)),(100,3,(60,80,93,230)),(92,2,(145,124,99,180)),(87,1,(140,168,178,75))]:
        D.ellipse((128-rad,128-rad,128+rad,128+rad),outline=col,width=w)
    for deg in range(0,360,30):
        th=math.radians(deg-90);p=lambda rad:(128+math.cos(th)*rad,128+math.sin(th)*rad)
        D.line((*p(99),*p(105)),fill=(222,194,137,195),width=2)
    for deg in (0,90,180,270):
        th=math.radians(deg-90);x=128+math.cos(th)*106;y=128+math.sin(th)*106
        d=8
        D.polygon([(x,y-d),(x+d,y),(x,y+d),(x-d,y)],fill=(60,62,61,255))
        D.line([(x,y-d),(x+d,y),(x,y+d),(x-d,y),(x,y-d)],fill=(239,212,154,235),width=2)
    # Build crisp center glyph from standalone SVG, never crop/source a contact sheet.
    svg=f'''<svg xmlns="http://www.w3.org/2000/svg" width="190" height="190" viewBox="0 0 190 190">
    <defs><linearGradient id="steel" x1="0" y1="0" x2="1" y2="1"><stop stop-color="#f4f0e5"/><stop offset="0.48" stop-color="#879baa"/><stop offset="1" stop-color="#cbdde1"/></linearGradient>
    <linearGradient id="bronze" x1="0" y1="0" x2="1" y2="1"><stop stop-color="#f5dfac"/><stop offset="0.48" stop-color="#986f48"/><stop offset="1" stop-color="#d6a46a"/></linearGradient></defs>
    <g transform="translate(95 95) scale(1.18)">{glyph_body(name)}</g></svg>'''
    glyph=Image.open(BytesIO(cairosvg.svg2png(bytestring=svg.encode(),output_width=190,output_height=190))).convert('RGBA')
    # soft halo for legibility against varied game scenes
    alpha=glyph.getchannel('A').filter(ImageFilter.GaussianBlur(7)).point(lambda a:int(a*.32))
    halo=Image.new('RGBA',glyph.size,(*rgb,0));halo.putalpha(alpha)
    tile.alpha_composite(halo,(33,33));tile.alpha_composite(glyph,(33,33))
    return tile

def make_atlas():
    atlas=Image.new('RGBA',(2048,1024),(0,0,0,0))
    for idx,(name,color) in enumerate(ICONS):
        atlas.alpha_composite(icon_sprite(name,color),((idx%8)*256,(idx//8)*256))
    atlas.save(ART/'Icons-PRO-PREVIEW.png')
    write_rwtx(ROOT/'RunicWheelIcons.rwtx',atlas)
    return atlas

def write_rwtx(path,img):
    w,h=img.size
    rgba=np.asarray(img.convert('RGBA'))
    # RAW D3DFMT_A8R8G8B8 on little endian => memory BGRA
    bgra=rgba[:,:,[2,1,0,3]]
    path.write_bytes(b'RWAT'+struct.pack('<II',w,h)+bgra.tobytes())
    print(path.name, img.size,hashlib.sha256(path.read_bytes()).hexdigest()[:16])

def frame_image():
    n=1024
    Y,X=np.ogrid[:n,:n]; cx=cy=512
    r=np.sqrt((X-cx)**2+(Y-cy)**2)
    alpha=np.where((r>445)&(r<497),175,0).astype(np.uint8)
    pixels=np.zeros((n,n,4),dtype=np.uint8)
    bronze=(33,39,43)
    for c,v in enumerate(bronze):pixels[:,:,c]=v
    pixels[:,:,3]=alpha
    frame=Image.fromarray(pixels,'RGBA')
    d=ImageDraw.Draw(frame,'RGBA')
    for rad,w,c in [(490,10,(37,41,46,245)),(488,4,(221,185,121,235)),(476,4,(77,84,86,215)),(469,2,(238,214,159,200)),(425,6,(38,40,44,210)),(422,3,(203,161,103,170))]:
        d.ellipse((512-rad,512-rad,512+rad,512+rad),outline=c,width=w)
    for deg in range(0,360,15):
        theta=math.radians(deg-90)
        a=472 if deg%45 else 462
        b=484 if deg%45 else 486
        d.line([(512+math.cos(theta)*a,512+math.sin(theta)*a),(512+math.cos(theta)*b,512+math.sin(theta)*b)],fill=(225,184,119,190),width=2)
    # Floating rune tracery inside the narrow annulus; opaque metal remains restrained.
    for i in range(64):
        angle=math.radians(i*360/64-90)
        cxr=512+math.cos(angle)*455;cyr=512+math.sin(angle)*455
        if i%2==0:
            offset=4 if i%4==0 else -4
            px=512+math.cos(angle+0.015)*450;py=512+math.sin(angle+0.015)*450
            d.line([(px,py),(cxr,cyr)],fill=(189,159,119,155),width=2)
    # keep each wedge visually distinct; metal dividers and inner engraved center
    for i in range(8):
        theta=math.radians(-112.5+i*45)
        xx,yy=math.cos(theta),math.sin(theta)
        d.line([(512+xx*170,512+yy*170),(512+xx*465,512+yy*465)],fill=(20,24,29,225),width=8)
        d.line([(512+xx*175,512+yy*175),(512+xx*462,512+yy*462)],fill=(199,165,115,185),width=3)
    d.ellipse((330,330,694,694),fill=(18,24,32,238),outline=(231,192,126,226),width=7)
    d.ellipse((349,349,675,675),outline=(103,123,136,235),width=4)
    d.ellipse((370,370,654,654),outline=(165,139,100,190),width=3)
    # original abstract rune compass in center, not a copied brand logo
    d.regular_polygon((512,512,115),8,rotation=22.5,outline=(115,136,140,100),width=2)
    for angle in range(0,360,45):
        theta=math.radians(angle)
        x0,y0=512+math.cos(theta)*121,512+math.sin(theta)*121
        x1,y1=512+math.cos(theta+0.12)*148,512+math.sin(theta+0.12)*148
        x2,y2=512+math.cos(theta-0.12)*148,512+math.sin(theta-0.12)*148
        d.line([(x1,y1),(x0,y0),(x2,y2)],fill=(205,167,110,130),width=3)
    for a in range(0,360,90):
        rad=math.radians(a)
        x1=512+math.cos(rad)*70;y1=512+math.sin(rad)*70
        d.arc((x1-19,y1-19,x1+19,y1+19),a,a+240,fill=(166,139,103,155),width=2)
    d.polygon([(512,415),(570,512),(512,609),(454,512)],outline=(244,215,157,220),width=6)
    d.polygon([(512,445),(550,512),(512,579),(474,512)],outline=(120,157,172,235),width=3)
    d.line([(512,452),(512,578)],fill=(226,203,162,205),width=4)
    for deg in range(0,360,45):
        a=math.radians(deg-90)
        x=512+math.cos(a)*493;y=512+math.sin(a)*493
        d.regular_polygon((x,y,13),4,rotation=45,fill=(37,46,51,245),outline=(221,183,119,240),width=2)
    frame.save(ART/'Frame-PRO-PREVIEW.png')
    write_rwtx(ROOT/'RunicWheelFrame.rwtx',frame)
    return frame

# 16 * 16 cells of 64px, fixed atlas aligned on 64px texture grid.
FONT_CODES=list(range(32,127))+list(range(160,256))+[0x0152,0x0153,0x20AC,0x2013,0x2014,0x2019,0x2018,0x201C,0x201D,0x2026,0x0178,0x0160,0x0161,0x017D,0x017E]
assert len(FONT_CODES)<256

def font_atlas():
    font=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSerif.ttf',46)
    atlas=Image.new('RGBA',(1024,1024),(255,255,255,0));d=ImageDraw.Draw(atlas,'RGBA')
    advances=[]
    for i,cp in enumerate(FONT_CODES):
        glyph=chr(cp)
        x=(i%16)*64;y=(i//16)*64
        bb=d.textbbox((0,0),glyph,font=font)
        advance=min(63,max(5,int(round(d.textlength(glyph,font=font)))))
        # left-side 4px padding; baseline 53; no glyph crosses 64px tile
        dx=x+4;dy=y+8-bb[1]
        d.text((dx,dy),glyph,font=font,fill=(255,255,255,255))
        advances.append(min(63,advance+3))
    assert len(advances)==len(FONT_CODES)
    atlas.save(ART/'Font-PRO-PREVIEW.png')
    write_rwtx(ROOT/'RunicWheelFont.rwtx',atlas)
    header=ROOT/'ProfessionalFontMap.h'
    header.write_text('#pragma once\n// Generated by art/generate.py; DejaVu Serif raster glyphs only, no font file distributed.\n'
       f'constexpr unsigned kProGlyphCount={len(FONT_CODES)}u;\n'
       +'static const unsigned short kProCodepoints[]={'+','.join(f'0x{cp:04X}' for cp in FONT_CODES)+'};\n'
       +'static const unsigned char kProAdvances[]={'+','.join(map(str,advances))+'};\n')
    return atlas

if __name__=='__main__':
    make_atlas(); frame_image();font_atlas()
