#include "SpellMembership.h"
#include <cstdio>
#include <cstdlib>
#define CHECK(cond) do{if(!(cond)){std::fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#cond);std::exit(1);}}while(0)
int main(){
    int a=0,b=0,c=0;void* player=&a;void* npc=&b;void* race=&c;
    void* actorList[]={npc};void* baseList[]={player};void* raceList[]={race};
    CHECK(rw12::contains(actorList,1,npc));
    CHECK(!rw12::contains(actorList,1,player));
    CHECK(rw12::contains(baseList,1,player));
    CHECK(rw12::contains(raceList,1,race));
    CHECK(!rw12::contains(baseList,1,race));
    CHECK(!rw12::contains(baseList,0,player));
    CHECK(!rw12::contains(baseList,4097,player));
    CHECK(!rw12::contains((void* const*)nullptr,1,player));
    CHECK(!rw12::contains(baseList,1,(void*)nullptr));
    CHECK(rw12::valid_count(4096)&&!rw12::valid_count(4097));
    std::puts("PASS spell membership: actor, base, race, fail-closed bounds");
}
