#include "InputLogic.h"
#include "ContextPolicy.h"
#include "BindingModel.h"
#include <cstdio>
#define CHECK(x) do{ if(!(x)){ std::fprintf(stderr,"FAIL at %d: %s\n",__LINE__,#x);return 1; } }while(0)
int main(){
    rw02::Activation a{};
    CHECK(rw02::button(a,true,1000,260));
    CHECK(rw02::button(a,false,1060,260)); // tap leaves wheel toggled
    CHECK(!rw02::button(a,true,2000,260));  // tap again closes
    rw02::button(a,false,2020,260);
    CHECK(rw02::button(a,true,3000,260));
    CHECK(!rw02::button(a,false,3350,260)); // hold-release closes
    CHECK(rw07::click(false,true,false,0x01000001)==rw07::None);
    CHECK(rw07::click(true,true,false,0)==rw07::None); // no hovered form: fail closed
    CHECK(rw07::click(true,true,false,0x01000001)==rw07::AddHovered);
    CHECK(rw07::click(true,true,true,0)==rw07::RemoveSelected);
    CHECK(rw07::click(true,false,false,0x01000001)==rw07::ApplySelected);
    rw05::State state{}; rw05::Editor history{};
    rw05::Binding first{}; first.formId=0x01000001;
    rw05::Binding second{}; second.formId=0x01000002;
    CHECK(rw05::insert(history,state,0,3,first));
    CHECK(rw05::insert(history,state,0,3,second)); // left click in filled sector stacks
    CHECK(state.slot[0][3].count==2);
    CHECK(!rw05::insert(history,state,0,3,first)); // duplicate cannot fill slots
    state.cursor[0][3]=1;
    CHECK(rw05::current(state,0,3).formId==second.formId);
    state.cursor[0][3]=rw02::step(state.cursor[0][3],-1,state.slot[0][3].count);
    CHECK(rw05::current(state,0,3).formId==first.formId);
    CHECK(rw05::erase(history,state,0,3,0)); // right click removes only selected item
    CHECK(state.slot[0][3].count==1 && rw05::current(state,0,3).formId==second.formId);
    CHECK(rw05::validate(state));
    std::puts("PASS: native gesture policy, short/long press, add/stack/remove/scroll without editor keys");
}
