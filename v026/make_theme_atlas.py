from PIL import Image, ImageDraw, ImageFilter, ImageEnhance
import struct, os
from pathlib import Path
source=str(Path(__file__).resolve().parent / 'REFERENCE' / 'FIRST-APPROVED-ICON-LANGUAGE.png')
dst=str(Path(__file__).resolve().parent)
os.makedirs(dst,exist_ok=True)
im=Image.open(source).convert('RGBA')
centers_x=[[85,251,424,605,786,971,1147,1329],[86,266,445,625,806,987,1168,1347]]
centers_y=[249,486]
atlas=Image.new('RGBA',(1024,1024),(0,0,0,0))
icons=[]
for i in range(16):
  cx=centers_x[i//8][i%8]; cy=centers_y[i//8]
  # Crop 168px roundel and remove checkerboard and captions outside its rune rim
  crop=im.crop((cx-84,cy-84,cx+84,cy+84))
  alpha=Image.new('L',(168,168),0)
  m=ImageDraw.Draw(alpha)
  m.ellipse((7,7,160,160),fill=255)
  alpha=alpha.filter(ImageFilter.GaussianBlur(2.1))
  crop.putalpha(alpha)
  crop=crop.resize((212,212),Image.Resampling.LANCZOS)
  crop=ImageEnhance.Sharpness(crop).enhance(1.13)
  x=(i%4)*256+22; y=(i//4)*256+22
  atlas.alpha_composite(crop,(x,y))
  icons.append(crop)
# Transparent art for inspection (delivery runtime is simple uncompressed BGRA texture)
atlas.save(f'{dst}/RunicWheelIcons-PREVIEW.png')
header=b'RWAT'+struct.pack('<II',1024,1024)
with open(f'{dst}/RunicWheelIcons.rwtx','wb') as f:
  f.write(header)
  f.write(atlas.tobytes('raw','BGRA'))
print('saved texture:',os.path.getsize(f'{dst}/RunicWheelIcons.rwtx'))
