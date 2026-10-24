#include "ActionPolicy.h"
#include "SaveV2.h"
#include "BindSyntax.h"
#include <cstdio>
#include <cstring>
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#x);return 1;}}while(0)
static bool remap(unsigned old,unsigned* now,void*) { if(old==0xFF001234)return false;*now=old+1;return true; }
int main(){
 using namespace rw05;
 State s{}; Editor e{};
 Binding weapon{};weapon.formId=0x01001234;weapon.variantId=0xAABBCCDD;weapon.kind=Weapon;weapon.hand=Right;
 CHECK(insert(e,s,0,0,weapon));CHECK(!insert(e,s,0,0,weapon));
 Binding second=weapon;second.variantId=0x11335577;CHECK(insert(e,s,0,0,second));
 CHECK(s.slot[0][0].count==2);
 CHECK(relocate(e,s,0,0,1,1,2,true));CHECK(s.slot[1][2].item[0].variantId==second.variantId);
 CHECK(relocate(e,s,0,0,0,2,5,false));CHECK(s.slot[0][0].count==1);
 CHECK(history(e,s,true));CHECK(s.slot[0][0].count==2);
 CHECK(history(e,s,false));CHECK(s.slot[0][0].count==1);
 CHECK(erase(e,s,0,0,0));CHECK(!erase(e,s,0,0,0));CHECK(validate(s));
 unsigned char bytes[PacketBytes]{};encode(s,bytes);
 CHECK(bytes[3]=='2');State loaded{};unsigned lost=99;
 CHECK(decode(bytes,PacketBytes,loaded,remap,nullptr,&lost));CHECK(lost==0);
 CHECK(loaded.slot[2][5].item[0].formId==weapon.formId+1);
 State prev=loaded;bytes[44]^=0x10;CHECK(!decode(bytes,PacketBytes,loaded,remap,nullptr));
 CHECK(loaded.slot[2][5].item[0].formId==prev.slot[2][5].item[0].formId);
 encode(s,bytes);bytes[17]=5; // malformed capacity and forced valid checksum, not merely CRC test
 unsigned crc=rw04::crc32(bytes,12);crc=~rw04::crc32(bytes+Header,PacketBytes-Header,crc);
 rw04::put32(bytes+12,crc);CHECK(!decode(bytes,PacketBytes,loaded,remap,nullptr));
 rw04::State old{};CHECK(rw03::add(old.catalog.slots[0][0],0x11223344));
 unsigned char oldbytes[rw04::kBytes]{};rw04::encode(old,oldbytes);
 CHECK(decode_any(oldbytes,rw04::kBytes,loaded,remap,nullptr));
 CHECK(loaded.slot[0][0].item[0].formId==0x11223345);
 CHECK(loaded.slot[0][0].item[0].kind==Unspecified);
 Context c{};c.focused=true;c.gameplay=true;c.actorReady=true;c.owned=true;c.resolvedKind=Weapon;
 CHECK(plan(weapon,c).reason==MissingVariant);
 c.exactVariantFound=true;CHECK(plan(weapon,c).operation==EquipObject);
 c.gameplay=false;CHECK(plan(weapon,c).operation==Reject);c.gameplay=true;
 Binding spell{};spell.formId=0x01004567;spell.kind=Spell;spell.hand=Left;
 c.resolvedKind=Spell;CHECK(plan(spell,c).reason==NotLearned);
 c.learned=true;CHECK(plan(spell,c).operation==EquipMagic);
 Binding shout{};shout.formId=0x01005678;shout.kind=Shout;shout.hand=Voice;
 c.resolvedKind=Shout;CHECK(plan(shout,c).operation==EquipVoice);
 Binding potion{};potion.formId=0x01009876;potion.kind=Consumable;potion.hand=Automatic;
 c.resolvedKind=Consumable;c.learned=false;CHECK(plan(potion,c).operation==Consume);
 Pending q{};reset(q,7);CHECK(enqueue(q,weapon,7));CHECK(!enqueue(q,weapon,7));
 Binding recv{};CHECK(!consume(q,8,recv));CHECK(consume(q,7,recv));CHECK(!consume(q,7,recv));
 CHECK(recv.variantId==weapon.variantId);reset(q,8);CHECK(!enqueue(q,weapon,7));
 // Force rejection on malformed binding rather than allowing unsafe native calls.
 weapon.hand=Voice;CHECK(!enqueue(q,weapon,8));
 rw05::Slot typed{};
 CHECK(rw05::parse_slot("01001234@AABBCCDD/W/R, 0100BEEF/S/L",typed)==0);
 CHECK(typed.count==2 && typed.item[0].variantId==0xAABBCCDD && typed.item[1].hand==Left);
 CHECK(rw05::parse_slot("01001234@/W/R",typed)==1);
 CHECK(rw05::parse_slot("01001234@AABBCCDD/W/V",typed)==1);
 CHECK(rw05::parse_slot("0x01001234/W/R, 0x01001234/W/R",typed)==1);
 CHECK(typed.count==1);
 std::puts("PASS v0.5 variant-aware bindings, editor transactions, V1 migration, V2 CRC/remap, action policy and queue");
}
