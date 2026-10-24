#include "LayoutGeometry.h"
#include <cassert>
int main(){
 for(int n=4;n<=8;++n){
    for(int i=0;i<n;++i){auto v=rwl24::center(n,i);
       assert(rwl24::select(n,(int)(v.x*200),(int)(v.y*200),20)==i);}
    assert(rwl24::select(n,0,0,22)==-1);
 }
 assert(rwl24::wrap_page(0,-1,3)==2);
 assert(rwl24::wrap_page(1,1,2)==0);
 assert(rwl24::visible_page(2,1)==0);
}
