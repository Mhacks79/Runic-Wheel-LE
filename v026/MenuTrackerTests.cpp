#include "MenuTracker.h"
#include <cstdlib>
#include <cstdio>
static void check(bool x){if(!x){std::fputs("FAIL context\n",stderr);std::exit(1);}}
int main(){
 check(rw08::identify("InventoryMenu")==1);
 check(rw08::identify("MagicMenu")==2);
 check(rw08::identify("HUD Menu")==0);
 check(rw08::identify("Inventorymenu")==0);
 check(!rw08::slowdown(false,0));
 check(rw08::slowdown(true,0));
 unsigned flags=rw08::transition(0,1,true);
 check(!rw08::slowdown(true,flags));
 flags=rw08::transition(flags,2,true);
 check(flags==3);
 flags=rw08::transition(flags,1,false);
 check(flags==2 && !rw08::slowdown(true,flags));
 flags=rw08::transition(flags,2,false);
 check(flags==0 && rw08::slowdown(true,flags));
 std::puts("PASS menu lifecycle context state and slowdown policy");
}
