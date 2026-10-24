#include "TimePolicy.h"
#include <cstdio>
int main(){
    int failed=0;
    auto check=[&](bool b,const char* name){if(!b){std::fprintf(stderr,"FAIL: %s\n",name);++failed;}};
    check(rw09::wants_slowdown(true,0,true),"world open = slow");
    check(!rw09::wants_slowdown(false,0,true),"closed = restore");
    check(!rw09::wants_slowdown(true,1,true),"inventory = no slow");
    check(!rw09::wants_slowdown(true,2,true),"magic = no slow");
    check(!rw09::wants_slowdown(true,0,false),"setting disabled");
    check(rw09::target_speed(1.f,10)>0.099f && rw09::target_speed(1.f,10)<0.101f,"0.1x");
    check(rw09::target_speed(.01f,10)==.01f,"clamp");
    check(rw09::still_owns_multiplier(.1f,.1f),"restore owned");
    check(!rw09::still_owns_multiplier(.3f,.1f),"other mod changed speed");
    if(failed)return 1;std::puts("PASS TimePolicy (9 cases)");
}
