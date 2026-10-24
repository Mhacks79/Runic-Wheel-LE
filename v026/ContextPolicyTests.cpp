#include "ContextPolicy.h"
#include "BindingModel.h"
#include "CatalogEditor.h"
#include "SaveV2.h"
#include <cstdio>
#include <cstdlib>
static void check(bool v,int line){if(!v){std::fprintf(stderr,"FAIL at %d\n",line);std::exit(1);}}
#define REQUIRE(x) check((x),__LINE__)
static bool resolver(unsigned id,unsigned* result,void*){*result=id;return true;}
int main(){
    REQUIRE(rw07::click(false,false,false,22)==rw07::None);
    REQUIRE(rw07::click(true,true,false,0)==rw07::None);
    REQUIRE(rw07::click(true,true,false,12)==rw07::AddHovered);
    REQUIRE(rw07::click(true,true,true,0)==rw07::RemoveSelected);
    REQUIRE(rw07::click(true,false,false,12)==rw07::ApplySelected);
    REQUIRE(rw07::menu_recent(1000,950));
    REQUIRE(!rw07::menu_recent(1000,200));
    REQUIRE(!rw07::menu_recent(1000,0));
    REQUIRE(rw07::menu_recent(0x10u,0xFFFFFFF0u)); // GetTickCount rollover
    REQUIRE(rw07::should_slow(true,false));
    REQUIRE(!rw07::should_slow(true,true));
    REQUIRE(!rw07::should_slow(false,false));
    rw05::State state{};rw05::Editor editor{};
    for(int i=0;i<4;i++){
        rw05::Binding b{};b.formId=0x0001397Eu+i;b.kind=rw05::Weapon;
        REQUIRE(rw05::insert(editor,state,0,0,b));
        state.cursor[0][0]=static_cast<unsigned char>(i);
    }
    REQUIRE(state.slot[0][0].count==4);
    rw05::Binding fifth{};fifth.formId=0x4321;fifth.kind=rw05::Weapon;
    REQUIRE(!rw05::insert(editor,state,0,0,fifth));
    REQUIRE(!rw05::insert(editor,state,0,0,state.slot[0][0].item[0]));
    REQUIRE(rw05::erase(editor,state,0,0,3));
    REQUIRE(state.slot[0][0].count==3);
    REQUIRE(rw05::history(editor,state,true));
    REQUIRE(state.slot[0][0].count==4);
    unsigned char bytes[rw05::PacketBytes]{};
    rw05::encode(state,bytes);
    rw05::State decoded{};
    REQUIRE(rw05::decode_any(bytes,sizeof(bytes),decoded,resolver,nullptr,nullptr));
    REQUIRE(decoded.slot[0][0].count==4);
    for(int i=0;i<4;i++)REQUIRE(decoded.slot[0][0].item[i].formId==0x0001397Eu+unsigned(i));
    std::puts("PASS: automatic context, rollover, slowdown policy, 4-entry stack, undo, co-save");
}
