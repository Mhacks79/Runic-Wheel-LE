#pragma once
// Device-facing input policy, host-testable; no TESV or Win32 dependencies.
// Wheel release events are NOT new scroll detents. In 0.17-0.19, treating
// value=0/duration=0 as an unconditional detent could immediately undo a
// previous press (most obvious with two items: 1/2 -> 2/2 -> 1/2).
namespace rw20 {
struct ScrollPulse {
    unsigned lastPositive=0, lastAccepted=0;
    bool hasPositive=false, hasAccepted=false;
};
inline bool accept(ScrollPulse& state,float value,float duration,unsigned now,unsigned intervalMs=18){
    if(value>0.f){
        state.lastPositive=now;
        state.hasPositive=true;
    }else{
        // Legitimate zero-value-only LE wheel feeds remain supported, but
        // a zero-valued release following a positive edge cannot cycle again.
        if(duration!=0.f || (state.hasPositive && (unsigned)(now-state.lastPositive)<400u))return false;
    }
    if(state.hasAccepted && (unsigned)(now-state.lastAccepted)<intervalMs)return false;
    state.lastAccepted=now;
    state.hasAccepted=true;
    return true;
}
inline void reset(ScrollPulse (&edges)[2]){edges[0]={};edges[1]={};}
inline unsigned next_filled(const unsigned char counts[8],unsigned current,int direction){
    if(current>=8 || !direction)return current;
    for(unsigned d=1;d<=8;++d){
        const unsigned candidate=(current+(direction<0 ? 8-d : d))%8;
        if(counts[candidate]>0 && counts[candidate]<=4)return candidate;
    }
    return current;
}
inline unsigned preview_index(unsigned current,unsigned count){
    return count>1 && count<=4 && current<count ? (current+1)%count : current;
}
}
