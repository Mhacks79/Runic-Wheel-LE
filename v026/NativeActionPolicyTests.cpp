#include "NativeActionPolicy.h"
#include <cstdio>
#include <cstdlib>
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x);std::exit(1);}}while(0)
int main(){
 rw05::Binding b{0x13790,0,rw05::Weapon,rw05::Automatic,0,0};
 rw11::Facts f{true,true,true,true,false,false,false,rw05::Weapon,1};
 CHECK(rw11::decide(b,f)==rw11::Equip);
 f.worn=true;CHECK(rw11::decide(b,f)==rw11::Unequip);
 f.instanceMissing=true;CHECK(rw11::decide(b,f)==rw11::Refuse);
 f.instanceMissing=false; f.availableCount=0;CHECK(rw11::decide(b,f)==rw11::Refuse);
 // v0.26: a bound copy (variantId) is resolved by the adapter, no longer refused outright.
 f.availableCount=1;f.worn=false;b.variantId=0x12345678;CHECK(rw11::decide(b,f)==rw11::Equip);
 f.instanceMissing=true;CHECK(rw11::decide(b,f)==rw11::Refuse);
 f.instanceMissing=false;
 b.variantId=0;b.kind=rw05::Armor;CHECK(rw11::decide(b,f)==rw11::Refuse); // stale kind
 b.kind=rw05::Unspecified;f.actualKind=rw05::Consumable;CHECK(rw11::decide(b,f)==rw11::Consume);
 f.actualKind=rw05::Spell;f.learned=false;CHECK(rw11::decide(b,f)==rw11::Refuse);
 f.learned=true;CHECK(rw11::decide(b,f)==rw11::EquipSpell);
 // v0.26: shouts have a native route once learned with a word unlocked.
 f.actualKind=rw05::Shout;CHECK(rw11::decide(b,f)==rw11::EquipShout);
 f.learned=false;CHECK(rw11::decide(b,f)==rw11::Refuse);
 // Scrolls are Unspecified in RWL2 and only act when the adapter saw formType 0x17.
 f.actualKind=rw05::Unspecified;f.learned=false;CHECK(rw11::decide(b,f)==rw11::Refuse);
 f.scroll=true;CHECK(rw11::decide(b,f)==rw11::Equip);
 f.worn=true;CHECK(rw11::decide(b,f)==rw11::Unequip);
 f.owned=false;CHECK(rw11::decide(b,f)==rw11::Refuse);
 f.owned=true;f.worn=false;f.scroll=false;
 f.actualKind=rw05::Weapon;f.focused=false;CHECK(rw11::decide(b,f)==rw11::Refuse);
 std::puts("PASS: 20 native action policy assertions");
}
