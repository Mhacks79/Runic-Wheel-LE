#include "PaletteEditor.h"
#include "SaveV2.h"
#include <cstdio>
#define CHECK(x) do { if(!(x)){std::fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#x);return 1;}}while(0)
static bool identity(unsigned in,unsigned* out,void*){*out=in;return true;}
int main(){
    using namespace rw06;
    Palette p{}; Ui u{};rw05::State state{};rw05::Editor editor{};
    rw05::Binding b{};b.formId=0x00012EB7; b.kind=rw05::Weapon;b.hand=rw05::Right;
    CHECK(add_palette(p,b));CHECK(!add_palette(p,b));
    b.formId=0x0001397E;b.kind=rw05::Unspecified;b.hand=rw05::Automatic;
    CHECK(add_palette(p,b));CHECK(p.count==2);
    CHECK(click(u,editor,state,p,0,0,false)==Added);
    CHECK(state.slot[0][0].item[0].formId==0x00012EB7);
    CHECK(click(u,editor,state,p,0,0,false)==Rejected); // cannot insert a duplicate
    cycle_palette(u,p,+1);CHECK(u.paletteIndex==1);
    CHECK(click(u,editor,state,p,0,0,false)==Added);
    CHECK(state.slot[0][0].count==2&&state.cursor[0][0]==1);
    CHECK(click(u,editor,state,p,0,1,false)==Added);
    CHECK(state.slot[0][1].item[0].formId==0x0001397E);
    cycle_tool(u);CHECK(u.tool==Move);
    CHECK(click(u,editor,state,p,0,0,false)==SelectedSource);
    CHECK(u.sourcePending);
    CHECK(click(u,editor,state,p,0,1,false)==Rejected); // duplicate in destination
    CHECK(u.sourcePending); // may retarget without losing source
    CHECK(click(u,editor,state,p,1,1,false)==Relocated);
    CHECK(!u.sourcePending && state.slot[1][1].count==1);
    cycle_tool(u);CHECK(u.tool==Copy);
    CHECK(click(u,editor,state,p,1,1,false)==SelectedSource);
    CHECK(click(u,editor,state,p,1,2,false)==Copied);
    CHECK(state.slot[1][1].count==1 && state.slot[1][2].count==1);
    CHECK(click(u,editor,state,p,1,2,true)==Removed);
    CHECK(state.slot[1][2].count==0);
    CHECK(rw05::history(editor,state,true));CHECK(state.slot[1][2].count==1);
    CHECK(rw05::history(editor,state,false));CHECK(state.slot[1][2].count==0);
    CHECK(rw05::validate(state));
    unsigned char blob[rw05::PacketBytes]{};rw05::encode(state,blob);
    rw05::State loaded{};CHECK(rw05::decode(blob,rw05::PacketBytes,loaded,identity,nullptr));
    CHECK(loaded.slot[1][1].count==1&&loaded.slot[0][0].count==1);
    blob[40]^=1;CHECK(!rw05::decode(blob,rw05::PacketBytes,loaded,identity,nullptr));
    CHECK(loaded.slot[1][1].count==1);
    // Every invalid input fails shut; all palette navigation is bounded.
    rw05::Binding invalid{};CHECK(!add_palette(p,invalid));
    for(int i=0;i<1024;++i){cycle_palette(u,p,i%2?-1:1);CHECK(u.paletteIndex<p.count);cycle_tool(u);CHECK(u.tool<=Copy);}
    CHECK(click(u,editor,state,p,15,0,false)==Rejected);
    std::puts("PASS v0.6: palette add/move/copy/delete, selection, undo/redo, V2 save, corruption and bounds");
}
