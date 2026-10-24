#include "StackNavigation.h"
#include "SaveV2.h"
#include "IconPolicy.h"
#include <cassert>
#include <cstring>
#include <cstdio>
static bool keep(unsigned id,unsigned* out,void*){*out=id;return true;}
int main(){
 rw05::State state{};
 auto& slot=state.slot[1][4];
 for(unsigned i=0;i<4;++i){rw05::Binding binding{};binding.formId=0x01000001+i;binding.kind=rw05::Spell;assert(rw05::add(slot,binding));}
 state.activeWheel=1;
 assert(rw05::validate(state));
 assert(rw19::cycle(state,1,4,+1));
 assert(state.cursor[1][4]==1 && rw05::current(state,1,4).formId==0x01000002);
 assert(rw19::cycle(state,1,4,+1));assert(state.cursor[1][4]==2);
 assert(rw19::cycle(state,1,4,+1));assert(state.cursor[1][4]==3);
 assert(rw19::cycle(state,1,4,+1));assert(state.cursor[1][4]==0);
 assert(rw19::cycle(state,1,4,-1));assert(state.cursor[1][4]==3);
 assert(state.cursor[1][3]==0 && state.cursor[0][4]==0);
 char txt[48]{};
 rw19::caption(txt, sizeof txt,"DESTRUCTION",state.cursor[1][4],slot.count);
 assert(std::strcmp(txt,"DESTRUCTION  [4/4]")==0);
 unsigned char packet[rw05::PacketBytes]{};
 rw05::encode(state,packet);
 rw05::State loaded{};
 assert(rw05::decode(packet,sizeof(packet),loaded,keep,nullptr));
 assert(loaded.cursor[1][4]==3 && rw05::current(loaded,1,4).formId==0x01000004);
 assert(!rw19::cycle(loaded,1,5,+1));
 assert(!rw19::cycle(loaded,3,5,+1));
 char tiny[5]{};
 rw19::caption(tiny,sizeof tiny,"RESTORATION",0,4);
 assert(std::strcmp(tiny,"REST")==0);
 assert(rw14::icon_for(0x16,0,0,0,0,3,0)==rw14::Power);
 assert(rw14::icon_for(0x77,0,0,0,0)==rw14::Shout);
 std::puts("PASS V019: wrap, reverse, stack isolation, caption bounds and co-save cursor roundtrip");
}
