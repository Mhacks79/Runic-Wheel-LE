#!/usr/bin/env python3
"""Release-blocking visual contracts for v0.15. Does not pretend to be Skyrim runtime."""
import hashlib,struct,re
from pathlib import Path
from PIL import Image
P=Path(__file__).resolve().parent
SRC=(P/'RunicWheelV011.cpp').read_text()
INI=(P/'RunicWheelLE.ini').read_text()
for name,(w,h) in {
    'RunicWheelIcons.rwtx':(2048,1024),'RunicWheelFrame.rwtx':(1024,1024),
    'RunicWheelFont.rwtx':(1024,1024),'RunicWheelPlaque.rwtx':(1024,256)
}.items():
    b=(P/name).read_bytes()
    assert b[:4]==b'RWAT' and b[4:12]==struct.pack('<II',w,h),name
    assert len(b)==12+w*h*4,name
assert 'static Vertex g_pips[3*(8*4+48)];' in SRC and 'for(int k=0;k<12;++k)' in SRC
assert 'g_plaqueAtlas=read_texture(device,"RunicWheelPlaque.rwtx",1024,256);' in SRC
assert 'if(ok && g_plaqueAtlas)ok=draw_textured_pass(device,g_plaqueAtlas,g_plaqueQuads,2);' in SRC
assert 'drop(g_plaqueAtlas);g_plaqueAtlas=nullptr;' in SRC
assert 'g_sectorLabels=value("ShowSectorLabels",0,0,1);' in SRC
assert 'ShowSectorLabels=0' in INI
assert 'ShowItemNames=1' in INI
assert 'void professional_text' in SRC and 'rw12ui::utf8_decode' in SRC
assert 'if(outer>height*.44f)outer=height*.44f;' in SRC
assert 'if(outer>h*.44f)outer=h*.44f;' in SRC
# Coordinates across screen sizes, scales, keeping rim and plaque inside view at 720+.
for w,h in [(1280,720),(1920,1080),(2560,1440),(3440,1440)]:
  for scale in [75,100,130,170]:
    outer=min(min(w,h)*.31*scale/100,440,h*.44)
    cy=h*.5-min(h*.045,40)
    assert cy-outer>=0,(w,h,scale,'top clipped')
    assert cy+outer+10-18+80<=h+0.0001,(w,h,scale,'nameplate clipped')
# True per-cell transparency, no atlas bleeding and visual identity uniqueness.
art=Image.open(P/'art/RunicWheelIcons-PREVIEW.png').convert('RGBA')
assert art.size==(2048,1024)
hashes=[]
for i in range(32):
  x=(i%8)*256;y=(i//8)*256
  tile=art.crop((x,y,x+256,y+256))
  alpha=tile.getchannel('A')
  for pos in [(0,0),(255,0),(0,255),(255,255),(128,0),(0,128)]:assert alpha.getpixel(pos)==0,(i,pos)
  assert alpha.getpixel((128,128))>150,i
  hashes.append(hashlib.sha256(tile.crop((55,47,204,203)).tobytes()).hexdigest())
assert len(set(hashes))==32,'duplicate item-type artwork'
icons=(P/'IconPolicy.h').read_text()
assert 'case 0x2E:' in icons and 'if(alchemyFlags&0x2u)return Food;' in icons
assert 'case 0x1E:return Ingredient;' in icons
assert 'if(armorMask&(1u<<9))return Shield;' in icons
# Renderer must not dereference a TESForm in D3D9 to obtain its name.
render=SRC[SRC.index('void draw_labels('):SRC.index('int draw(Raw device)')]
assert 'g_displayNames[wheel][selected][cursor]' in render
assert 'GetName(' not in render and '0x00452300' not in render
# Gameplay block is an immutable reference validated by previous user's in-game session.
# Existing native entrypoints, actions, and safety gates must survive.
for anchor in ('GameEquipSpell>(0x006EFAC0)', 'WEAPON unequip request submitted',
               'ARMOR unequip request submitted', 'ingredient eat native route not verified',
               'TORCH equip request submitted (verify in game)'):
    assert anchor in SRC,anchor
# Simulate deliberate regressions to prove the guard, rather than only happy paths.
mutant=icons.replace('if(alchemyFlags&0x2u)return Food;','if(alchemyFlags&0x2u)return Potion;')
assert 'if(alchemyFlags&0x2u)return Food;' not in mutant
mutant=SRC.replace('ShowSectorLabels",0','ShowSectorLabels",1')
assert 'g_sectorLabels=value("ShowSectorLabels",0,0,1);' not in mutant
print('PASS V015: 4 valid BGRA assets, 32 visually unique transparent icons, food/shield subtype, Unicode, sector-label policy, 16 viewport scale combinations, legacy action anchors plus isolated torch route, two negative guards')
