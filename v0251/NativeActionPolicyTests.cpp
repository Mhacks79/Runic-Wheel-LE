#include "NativeActionPolicy.h"
#include <cstdio>
#include <cstdlib>
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x);std::exit(1);}}while(0)
int main(){
 rw05::Binding b{0x13790,0,rw05::Weapon,rw05::Automatic,0,0};
 rw11::Facts f{true,true,true,true,false,false,false,rw05::Weapon,1};
 CHECK(rw11::decide(b,f)==rw11::Equip);
 f.worn=true;CHECK(rw11::decide(b,f)==rw11::Unequip);
 f.ambiguous=true;CHECK(rw11::decide(b,f)==rw11::Refuse);
 f.ambiguous=false; f.availableCount=0;CHECK(rw11::decide(b,f)==rw11::Refuse);
 f.availableCount=1;b.variantId=0x12345678;CHECK(rw11::decide(b,f)==rw11::Refuse);
 b.variantId=0;b.kind=rw05::Armor;CHECK(rw11::decide(b,f)==rw11::Refuse);
 b.kind=rw05::Unspecified;f.actualKind=rw05::Consumable;CHECK(rw11::decide(b,f)==rw11::Consume);
 f.actualKind=rw05::Spell;f.learned=false;CHECK(rw11::decide(b,f)==rw11::Refuse);
 f.learned=true;CHECK(rw11::decide(b,f)==rw11::EquipSpell);
 f.actualKind=rw05::Shout;CHECK(rw11::decide(b,f)==rw11::Refuse);
 f.actualKind=rw05::Weapon;f.focused=false;CHECK(rw11::decide(b,f)==rw11::Refuse);
 std::puts("PASS: 11 native action policy assertions");
}
