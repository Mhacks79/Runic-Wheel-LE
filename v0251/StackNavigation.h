#pragma once
// Host-testable model policy. This code never touches TESV or DirectX.
#include "BindingModel.h"
namespace rw19 {
inline bool cycle(rw05::State& s,unsigned wheel,unsigned sector,int direction){
    if(wheel>=rw05::Wheels || sector>=rw05::Sectors || direction==0)return false;
    rw05::Slot& slot=s.slot[wheel][sector];
    if(slot.count<2 || slot.count>rw05::Capacity)return false;
    const unsigned current=s.cursor[wheel][sector]<slot.count?s.cursor[wheel][sector]:0;
    const int length=(int)slot.count;
    const int next=((int)current + (direction<0?-1:1) + length)%length;
    s.cursor[wheel][sector]=(unsigned char)next;
    return next!=(int)current;
}
inline void caption(char* out,unsigned capacity,const char* label,unsigned index,unsigned count){
    if(!out || capacity==0)return;
    unsigned p=0;
    if(label)while(*label && p+1<capacity)out[p++]=*label++;
    if(count>1 && count<=rw05::Capacity && index<count && p+7<capacity){
        out[p++]=' ';out[p++]=' ';out[p++]='[';
        out[p++]=(char)('1'+index);out[p++]='/';out[p++]=(char)('0'+count);
        out[p++]=']';
    }
    out[p]=0;
}
inline unsigned caption_item_count(unsigned char count){return count<=rw05::Capacity?count:0;}
}
