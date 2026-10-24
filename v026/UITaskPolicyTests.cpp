#include "UITaskPolicy.h"
#include <cstdio>
int main(){
    int failures=0;
    auto test=[&](bool yes,const char* label){if(!yes){std::fprintf(stderr,"FAIL %s\n",label);++failures;}};
    test(!rw10::valid_capacity(0),"zero size rejected");
    test(rw10::valid_capacity(1),"one valid");
    test(rw10::valid_capacity(256),"normal valid");
    test(rw10::valid_capacity(2048),"limit valid");
    test(!rw10::valid_capacity(2049),"upper bound");
    test(!rw10::valid_capacity(7),"nonpower2 rejected");
    test(!rw10::sample_allowed(0,true,500),"no menu");
    test(!rw10::sample_allowed(1,false,500),"disabled");
    test(!rw10::sample_allowed(1,true,89),"rate limited");
    test(rw10::sample_allowed(1,true,90),"inventory");
    test(rw10::sample_allowed(2,true,500),"magic");
    test(!rw10::sample_allowed(4,true,500),"other menu");
    if(failures)return 1;
    std::puts("PASS: UI task policy (12 cases)");
}
