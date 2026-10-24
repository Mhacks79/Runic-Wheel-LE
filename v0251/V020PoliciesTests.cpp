#include "ScrollPulsePolicy.h"
#include "StackNavigation.h"
#include "SaveV2.h"
#include <cassert>
#include <cstdio>
static bool keep(unsigned id,unsigned* out,void*){*out=id;return true;}
int main(){
 using namespace rw20;
 ScrollPulse pulse{};
 // A press-release must produce one detent, never two.
 assert(accept(pulse,1.f,0.f,100));
 assert(!accept(pulse,0.f,0.f,100));
 assert(!accept(pulse,0.f,0.03f,150));
 assert(accept(pulse,1.f,0.f,220));
 assert(!accept(pulse,0.f,0.f,221));
 assert(!accept(pulse,1.f,0.f,225));
 assert(accept(pulse,1.f,0.f,260));
 // Zero-value-only feeds still work, but are rate-bounded.
 ScrollPulse zero{};
 assert(accept(zero,0.f,0.f,50));
 assert(!accept(zero,0.f,0.f,51));
 assert(accept(zero,0.f,0.f,100));
 // Positive press plus zero release = one step on a TWO ITEM stack.
 rw05::State state{};
 for(unsigned i=0;i<2;++i){rw05::Binding b{};b.formId=0x00003000+i;b.kind=rw05::Spell;assert(rw05::add(state.slot[0][3],b));}
 ScrollPulse two{};
 if(accept(two,1.f,0.f,100)) assert(rw19::cycle(state,0,3,1));
 if(accept(two,0.f,0.f,101)) assert(rw19::cycle(state,0,3,1));
 assert(state.cursor[0][3]==1);
 assert(rw05::current(state,0,3).formId==0x00003001);
 unsigned char packed[rw05::PacketBytes]{};rw05::encode(state,packed);
 rw05::State loaded{};assert(rw05::decode(packed,sizeof packed,loaded,keep,nullptr));
 assert(loaded.cursor[0][3]==1 && rw05::current(loaded,0,3).formId==0x00003001);
 assert(preview_index(1,2)==0 && preview_index(3,4)==0 && preview_index(0,1)==0);
 unsigned char counts[8]{0,2,0,4,0,0,1,0};
 assert(next_filled(counts,1,1)==3);
 assert(next_filled(counts,1,-1)==6);
 assert(next_filled(counts,6,1)==1);
 assert(next_filled(counts,3,-1)==1);
 unsigned char none[8]{};
 assert(next_filled(none,3,1)==3);
 std::puts("PASS V020: press/release, rapid pulses, zero-only fallback, 2-entry undo bug, wrap, preview, occupied sector navigation, co-save roundtrip");
}
