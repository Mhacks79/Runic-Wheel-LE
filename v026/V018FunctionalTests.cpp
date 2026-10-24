#include "SaveV2.h"
#include "NativeActionPolicy.h"
#include "IconPolicy.h"
#include <cstdio>
#define CHECK(x) do { if(!(x)) { std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x); return 1; } } while(0)
int main(){
  rw05::Binding t{0x0001D4EC,0,rw05::Torch,rw05::Automatic,0,0};
  CHECK(rw05::valid_binding(t));
  CHECK(rw14::icon_for(0x1F,0,0,0,0)==rw14::Torch);
  CHECK(rw14::icon_for(0x16,0,0,0,0,0,18)==rw14::Alteration);
  CHECK(rw14::icon_for(0x16,0,0,0,0,0,19)==rw14::Conjuration);
  CHECK(rw14::icon_for(0x16,0,0,0,0,0,20)==rw14::Destruction);
  CHECK(rw14::icon_for(0x16,0,0,0,0,0,21)==rw14::Illusion);
  CHECK(rw14::icon_for(0x16,0,0,0,0,0,22)==rw14::Restoration);
  CHECK(rw14::icon_for(0x16,0,0,0,0,3,0)==rw14::Power);
  CHECK(rw14::icon_for(0x16,0,0,0,0,7,0)==rw14::Shout);
  rw05::State s{};
  CHECK(rw05::add(s.slot[0][0],t));
  CHECK(rw05::validate(s));
  unsigned char save[rw05::PacketBytes]{};rw05::encode(s,save);
  CHECK(save[4]==2 && save[5]==3 && save[7]==rw05::Capacity);
  rw11::Facts facts{true,true,true,true,false,false,false,rw05::Torch,1};
  CHECK(rw11::decide(t,facts)==rw11::Equip);
  facts.worn=true;CHECK(rw11::decide(t,facts)==rw11::Unequip);
  facts.worn=false;facts.instanceMissing=true;CHECK(rw11::decide(t,facts)==rw11::Refuse);
  t.hand=rw05::Left;CHECK(!rw05::valid_binding(t));
  std::puts("PASS V018 torch + spell-school/power icon policy + serialize/gate");
}
