#pragma once
// Bounded Unicode-safe display names. Accepts UTF-8 and legacy CP1252 input;
// refuses malformed sequences and truncates only at codepoint boundaries.
namespace rw12ui {
constexpr unsigned NameBytes=72;
inline unsigned length_bounded(const char* s,unsigned cap){unsigned k=0;while(s&&k<cap&&s[k])++k;return k;}
inline unsigned utf8_decode(const unsigned char* s,unsigned max,unsigned& consumed){
 consumed=1;if(!max || !s[0])return 0;
 const unsigned c=s[0];if(c<128)return c;
 if((c&0xe0)==0xc0&&max>=2&&(s[1]&0xc0)==0x80){unsigned v=((c&31)<<6)|(s[1]&63);if(v>=128){consumed=2;return v;}}
 if((c&0xf0)==0xe0&&max>=3&&(s[1]&0xc0)==0x80&&(s[2]&0xc0)==0x80){unsigned v=((c&15)<<12)|((s[1]&63)<<6)|(s[2]&63);if(v>=2048&&!(v>=0xd800&&v<=0xdfff)){consumed=3;return v;}}
 // Standalone 0x80..0xFF from Skyrim's legacy Windows-1252 translated to Unicode.
 if(c>=0xa0)return c;
 switch(c){case 0x80:return 0x20ac;case 0x8c:return 0x0152;case 0x9c:return 0x0153;
 case 0x91:return 0x2018;case 0x92:return 0x2019;case 0x93:return 0x201c;case 0x94:return 0x201d;
 case 0x96:return 0x2013;case 0x97:return 0x2014;case 0x85:return 0x2026;default:return 0xfffd;}
}
inline unsigned unicode_upper(unsigned cp){
 if(cp>='a'&&cp<='z')return cp-32;
 if((cp>=0xe0&&cp<=0xf6)||(cp>=0xf8&&cp<=0xfe))return cp-32;
 if(cp==0xff)return 0x0178;
 if(cp==0x153)return 0x152;
 return cp;
}
inline unsigned encode_utf8(unsigned cp,char* dest){
 if(cp<0x80){dest[0]=(char)cp;return 1;}
 if(cp<0x800){dest[0]=(char)(0xc0|(cp>>6));dest[1]=(char)(0x80|(cp&63));return 2;}
 dest[0]=(char)(0xe0|(cp>>12));dest[1]=(char)(0x80|((cp>>6)&63));dest[2]=(char)(0x80|(cp&63));return 3;
}
inline void normalise_name(const char* in,char* out){
 unsigned i=0,n=0;out[0]=0;if(!in)return;
 while(i<64&&n+4<NameBytes){
   const unsigned char* p=(const unsigned char*)in+i;
   if(!p[0])break;
   unsigned used=1;unsigned cp=utf8_decode(p,64-i,used);i+=used;
   if(!cp)break;
   if(cp<32)continue;
   if(cp=='\t'||cp=='\n')cp=' ';
   // Preserve readable punctuation including apostrophes, UTF-8 accents, en dash.
   if(cp==0xfffd)cp='?';
   cp=unicode_upper(cp);
   if(cp==' '&&(n==0||out[n-1]==' '))continue;
   char encoded[3];unsigned size=encode_utf8(cp,encoded);
   if(n+size>=NameBytes)break;
   for(unsigned k=0;k<size;k++)out[n++]=encoded[k];
 }
 while(n&&out[n-1]==' ')--n;
 out[n]=0;
}
}
