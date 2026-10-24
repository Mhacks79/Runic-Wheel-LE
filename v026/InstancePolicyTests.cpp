// Host tests for which owned copy the wheel acts on (v0.26). Always-on checks (F16).
#include "InstancePolicy.h"
#include "KhaosPointerPolicy.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x);std::exit(1);}}while(0)
using namespace rw26;
static InstanceInfo copy(unsigned hash,int count,bool custom,bool right=false,bool left=false){
    InstanceInfo i{};i.hash=hash;i.count=count;i.customized=custom;i.wornRight=right;i.wornLeft=left;return i;
}
int main(int argc,char** argv){
    // CRC: the values zlib.crc32 gives (checked again from Python by V026Contracts.py).
    CHECK(crc32("",0)==0u);
    CHECK(crc32("123456789",0)==0xCBF43926u);
    CHECK(item_hash("Iron Sword",0x00012EB7)==crc32("Iron Sword",0x00012EB7));
    CHECK(item_hash("Iron Sword",0x05012EB7)==item_hash("Iron Sword",0x00012EB7)); // load index ignored, like SKSE
    if(argc==3){std::printf("%08X\n",crc32(argv[1],(unsigned)std::strtoul(argv[2],nullptr,16)));return 0;}

    const unsigned base=0x11111111u, fine=0x22222222u, named=0x33333333u;
    // One plain sword: plain choice for a plain or legacy binding.
    {InstanceInfo l[1]={copy(base,1,false)};
     CHECK(choose(0,base,1,l,1).pick==PickPlain);
     CHECK(choose(base,base,1,l,1).pick==PickPlain);
     CHECK(choose(fine,base,1,l,1).pick==PickNone);} // tempered copy sold: never substitute
    // 1 plain (no list) + 1 tempered (own list): each binding gets its own copy.
    {InstanceInfo l[1]={copy(fine,1,true)};
     CHECK(choose(base,base,2,l,1).pick==PickPlain);
     Choice c=choose(fine,base,2,l,1);CHECK(c.pick==PickList && c.index==0);
     CHECK(choose(0,base,2,l,1).pick==PickPlain);}
    // Only customized copies: a legacy binding still works, a plain binding does not.
    {InstanceInfo l[2]={copy(fine,1,true),copy(named,1,true)};
     Choice c=choose(0,base,2,l,2);CHECK(c.pick==PickList && c.index==0);
     CHECK(choose(base,base,2,l,2).pick==PickNone);
     c=choose(named,base,2,l,2);CHECK(c.pick==PickList && c.index==1);}
    // ExtraCount: three plain copies sharing a list with ownership data, one tempered.
    {InstanceInfo l[2]={copy(base,3,false),copy(fine,1,true)};
     CHECK(choose(base,base,4,l,2).pick==PickPlain);
     InstanceInfo m[1]={copy(fine,2,true)}; // two tempered in one stack, nothing plain
     CHECK(choose(base,base,2,m,1).pick==PickNone);
     CHECK(choose(fine,base,2,m,1).pick==PickList);}
    // Enchanted without its own name hashes as the base: found when nothing plain is left.
    {InstanceInfo l[1]={copy(base,1,true)};
     Choice c=choose(base,base,1,l,1);CHECK(c.pick==PickList && c.index==0);}
    CHECK(choose(base,base,0,nullptr,0).pick==PickNone);

    // Hand moves.
    {InstanceInfo l[2]={copy(base,1,false,true),copy(fine,1,true)};
     Choice plain=choose(base,base,2,l,2), tempered=choose(fine,base,2,l,2);
     CHECK(hand_move(plain,l,2,false,-1)==MoveEquip);
     CHECK(hand_move(plain,l,2,true,0)==MoveUnequip);   // same plain copy in hand: toggle off
     CHECK(hand_move(tempered,l,2,true,0)==MoveEquip);  // plain in hand, tempered wanted: swap
     CHECK(hand_move(plain,l,2,true,-1)==MoveRefuse);   // in hand but worn list unknown
     CHECK(hand_move(tempered,l,2,true,-1)==MoveEquip);
     l[0].wornRight=false;l[1].wornRight=true;
     CHECK(hand_move(tempered,l,2,true,1)==MoveUnequip);
     CHECK(hand_move(plain,l,2,true,1)==MoveEquip);     // tempered in hand, plain wanted: swap
     CHECK(hand_move(Choice{},l,2,false,-1)==MoveRefuse);}
    // Worn moves (armor, torch).
    {InstanceInfo l[2]={copy(base,1,false),copy(fine,1,true,true)};
     int w=-2;
     CHECK(worn_move(choose(fine,base,2,l,2),l,2,&w)==MoveUnequip && w==1);
     CHECK(worn_move(choose(base,base,2,l,2),l,2,&w)==MoveEquip && w==-1);
     l[0].wornRight=true;l[1].wornRight=false;
     CHECK(worn_move(choose(base,base,2,l,2),l,2,&w)==MoveUnequip && w==0);
     CHECK(worn_move(choose(fine,base,2,l,2),l,2,&w)==MoveEquip);
     CHECK(worn_move(Choice{},l,2,&w)==MoveRefuse);}

    // What may be added.
    const unsigned ok[]={0x29,0x1A,0x2A,0x2E,0x1E,0x1F,0x17,0x77};
    for(unsigned t:ok)CHECK(addable(t,0xFFFFFFFFu));
    CHECK(addable(0x16,0)&&addable(0x16,2)&&addable(0x16,3));
    CHECK(!addable(0x16,4)&&!addable(0x16,1)&&!addable(0x16,5)&&!addable(0x16,6)&&!addable(0x16,7));
    const unsigned no[]={0x1B,0x20,0x2D,0x34,0x00,0x2B,0x15};
    for(unsigned t:no)CHECK(!addable(t,0));
    CHECK(keeps_variant(0x29)&&keeps_variant(0x1A)&&!keeps_variant(0x2A)&&!keeps_variant(0x2E)&&!keeps_variant(0x17));
    CHECK(spell_takes_hand(0)&&!spell_takes_hand(2)&&!spell_takes_hand(3));

    // KHA-OS pointer helpers.
    rw26ui::Mapping m=rw26ui::client_to_buffer(1280,720,2560,1440);
    CHECK(m.sx==2.f && m.sy==2.f);
    m=rw26ui::client_to_buffer(1920,1080,1920,1080);CHECK(m.sx==1.f && m.sy==1.f);
    m=rw26ui::client_to_buffer(0,0,1920,1080);CHECK(m.sx==1.f && m.sy==1.f);
    m=rw26ui::client_to_buffer(100,100,4000,4000);CHECK(m.sx==1.f);
    CHECK(rw26ui::os_pointer_visible(1,true));
    CHECK(!rw26ui::os_pointer_visible(0,true));
    CHECK(!rw26ui::os_pointer_visible(1,false));
    CHECK(!rw26ui::os_pointer_visible(3,true));
    std::puts("PASS: v0.26 item copy choice, hand/worn moves, add filter, CRC, pointer mapping");
    return 0;
}
