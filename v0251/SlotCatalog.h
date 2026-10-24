#pragma once
// Runic Wheel LE — original compact catalog; no dynamic allocation, CRT or STLs.
// Three wheels, eight slots, up to four distinct FormIDs per slot.
namespace rw03 {
static constexpr int Wheels=3, Slots=8, Entries=4;
struct Slot { unsigned form[Entries]; unsigned char count; };
struct Catalog { Slot slots[Wheels][Slots]; };
inline bool hex_digit(char ch,unsigned& v){
 if(ch>='0'&&ch<='9')v=(unsigned)(ch-'0');
 else if(ch>='a'&&ch<='f')v=(unsigned)(ch-'a'+10);
 else if(ch>='A'&&ch<='F')v=(unsigned)(ch-'A'+10);
 else return false;
 return true;
}
// Reject invalid tokens instead of silently turning mistakes into FormID zero.
inline bool parse_hex(const char* text,unsigned& out){
 if(!text)return false;
 while(*text==' '||*text=='\t')++text;
 if(text[0]=='0'&&(text[1]=='x'||text[1]=='X'))text+=2;
 unsigned v=0;int digits=0;
 for(;;++text){unsigned n=0;
  if(!hex_digit(*text,n))break;
  if(++digits>8)return false;
  v=(v<<4)|n;
 }
 while(*text==' '||*text=='\t')++text;
 if(*text || digits==0 || v==0)return false;
 out=v;return true;
}
inline bool add(Slot& slot,unsigned id){
 if(!id || slot.count>=Entries)return false;
 for(unsigned n=0;n<slot.count;++n)if(slot.form[n]==id)return false;
 slot.form[slot.count++]=id;return true;
}
// Parse comma-separated hexadecimal FormIDs; malformed IDs are skipped, not truncated.
inline int parse_slot(const char* line,Slot& slot){
 slot={};if(!line)return 0;
 int skipped=0;
 for(unsigned offset=0;line[offset];){
  char token[32]={};unsigned t=0;bool overlong=false;
  while(line[offset] && line[offset]!=','){
   if(t+1<sizeof(token))token[t++]=line[offset];
   else { // consume overlong token and mark invalid
    while(line[offset]&&line[offset]!=',')++offset;
    ++skipped;overlong=true;break;
   }
   ++offset;
  }
  if(t&&!overlong){unsigned id=0;if(!parse_hex(token,id)||!add(slot,id))++skipped;}
  if(line[offset]==',')++offset;
 }
 return skipped;
}
// Editor-ready operations: bounded, stable, no allocations, no mutation on failure.
inline bool remove(Slot& slot,int index){
 if(index<0 || index>=slot.count)return false;
 for(int i=index+1;i<slot.count;++i)slot.form[i-1]=slot.form[i];
 slot.form[--slot.count]=0;return true;
}
inline bool swap_entries(Slot& slot,int a,int b){
 if(a<0 || b<0 || a>=slot.count || b>=slot.count)return false;
 unsigned x=slot.form[a];slot.form[a]=slot.form[b];slot.form[b]=x;return true;
}
inline bool swap_slots(Catalog& book,int wheel,int a,int b){
 if(wheel<0 || wheel>=Wheels || a<0 || a>=Slots || b<0 || b>=Slots)return false;
 Slot x=book.slots[wheel][a];book.slots[wheel][a]=book.slots[wheel][b];book.slots[wheel][b]=x;return true;
}
inline bool swap_wheels(Catalog& book,int a,int b){
 if(a<0 || b<0 || a>=Wheels || b>=Wheels)return false;
 for(int i=0;i<Slots;++i){Slot tmp=book.slots[a][i];book.slots[a][i]=book.slots[b][i];book.slots[b][i]=tmp;}
 return true;
}
inline int wrap(int value,int count){if(count<=0)return 0;int k=value%count;return k<0?k+count:k;}
inline unsigned entry(const Catalog& catalog,int wheel,int sector,int cursor){
 if(wheel<0||wheel>=Wheels||sector<0||sector>=Slots)return 0;
 const Slot& s=catalog.slots[wheel][sector];
 return s.count ? s.form[wrap(cursor,s.count)] : 0;
}
}
