#include "AdvancedEditor.h"
#include "SaveV2.h"
#include <cassert>
#include <cstdio>
using namespace rw05;
using namespace rw21;
static unsigned seed=0xA11CE55u;
static unsigned rand32(){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;}
static bool identity(unsigned from,unsigned* to,void*){*to=from;return from!=0;}
static bool equal_state(const State& a,const State& b){
 if(a.activeWheel!=b.activeWheel)return false;
 for(unsigned w=0;w<Wheels;++w)for(unsigned s=0;s<Sectors;++s){
  if(a.cursor[w][s]!=b.cursor[w][s] || a.slot[w][s].count!=b.slot[w][s].count)return false;
  for(unsigned i=0;i<a.slot[w][s].count;++i){
   if(!equal(a.slot[w][s].item[i],b.slot[w][s].item[i]))return false;
  }
 }
 return true;
}
int main(){
 State s{};Editor h{};Pending p{};
 unsigned next=0x01000000;
 unsigned accepted=0,encoded=0;
 for(unsigned step=0;step<35000;++step){
  const unsigned wheel=rand32()%Wheels,sector=rand32()%Sectors;
  const unsigned dstW=rand32()%Wheels,dstS=rand32()%Sectors;
  const unsigned choice=rand32()%15;
  bool ok=false;
  if(choice<=2){Binding b{};b.formId=++next;b.kind=Weapon;ok=insert(h,s,wheel,sector,b);}
  else if(choice==3)ok=erase(h,s,wheel,sector,s.cursor[wheel][sector]);
  else if(choice==4){
   const Mode mode=(rand32()%2)?MoveItem:CopyItem;
   if(arm(p,s,mode,wheel,sector)==Armed){
    ok=paste(p,h,s,dstW,dstS)==Applied;
    cancel(p);
   }
  }else if(choice==5){
   if(arm(p,s,SwapSector,wheel,sector)==Armed){ok=paste(p,h,s,dstW,dstS)==Applied;cancel(p);}
  }else if(choice==6){
   if(arm(p,s,CloneSector,wheel,sector)==Armed){ok=paste(p,h,s,dstW,dstS)==Applied;cancel(p);}
  }else if(choice==7)ok=clear_stack(h,s,wheel,sector);
  else if(choice==8)ok=rotate_entry(h,s,wheel,sector,(rand32()%2)?1:-1);
  else if(choice==9)ok=clone_wheel(h,s,wheel,dstW);
  else if(choice==10)ok=swap_wheels(h,s,wheel,dstW);
  else if(choice==11)ok=clear_wheel(h,s,wheel);
  else if(choice==12)ok=history(h,s,true);
  else if(choice==13)ok=history(h,s,false);
  else { const unsigned count=s.slot[wheel][sector].count;
   if(count)s.cursor[wheel][sector]=(unsigned char)(rand32()%count);
  }
  if(ok)++accepted;
  assert(validate(s));
  if(step%17==0){
    unsigned char packet[PacketBytes]{};encode(s,packet);
    State decoded{};assert(decode_any(packet,PacketBytes,decoded,identity,nullptr));
    assert(equal_state(s,decoded));++encoded;
  }
 }
 std::printf("PASS 35,000 randomized edits; accepted=%u serialization roundtrips=%u\n",accepted,encoded);
}
