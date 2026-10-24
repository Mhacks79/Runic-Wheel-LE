#pragma once
// Runic Wheel LE 0.16: renderer-only animation policy. No TESV/Game API access.
// uint32 subtraction deliberately handles GetTickCount rollover.
#include <stdint.h>
namespace rw16 {
struct Frame {float opacity, scale; bool draw;};
inline unsigned elapsed(uint32_t now,uint32_t start){return now-start;}
inline float saturate(float value){return value<0.f?0.f:(value>1.f?1.f:value);}
inline float progress(uint32_t now,uint32_t start,unsigned duration){
    return duration?saturate((float)elapsed(now,start)/(float)duration):1.f;
}
inline float ease_out(float t){t=saturate(t);const float r=1.f-t;return 1.f-r*r*r;}
inline float ease_in_out(float t){t=saturate(t);return t<.5f?4.f*t*t*t:1.f-4.f*(1.f-t)*(1.f-t)*(1.f-t);}
inline Frame frame(bool visible,bool closing,bool enabled,bool reducedMotion,
                   uint32_t now,uint32_t openTick,uint32_t closeTick,
                   unsigned openDuration,unsigned closeDuration){
    if(visible){
        if(!enabled || !openDuration)return {1.f,1.f,true};
        const float p=ease_out(progress(now,openTick,openDuration));
        return {p,reducedMotion?1.f:.966f+.034f*p,true};
    }
    if(!closing || !enabled || !closeDuration)return {0.f,1.f,false};
    const float p=progress(now,closeTick,closeDuration);
    if(p>=1.f)return {0.f,1.f,false};
    const float remaining=1.f-ease_in_out(p);
    return {remaining,reducedMotion?1.f:1.f-.018f*(1.f-remaining),true};
}
inline float selected_blend(uint32_t now,uint32_t start,unsigned duration,bool enabled,bool reducedMotion){
    return (!enabled || reducedMotion || !duration)?1.f:ease_out(progress(now,start,duration));
}
inline float feedback_alpha(uint32_t now,uint32_t start,unsigned duration){
    if(!duration || elapsed(now,start)>=duration)return 0.f;
    const unsigned fade=duration<480u?duration/2u:240u;
    const unsigned remaining=duration-elapsed(now,start);
    return fade && remaining<fade?(float)remaining/(float)fade:1.f;
}
inline uint32_t with_alpha(uint32_t color,float multiplier){
    const uint32_t a=(color>>24)&255u;
    return (color&0x00ffffffu)|((uint32_t)(a*saturate(multiplier)+.5f)<<24);
}
inline uint32_t lerp_color(uint32_t base,uint32_t active,float weight){
    weight=saturate(weight);
    uint32_t out=0;
    for(unsigned shift=0;shift<32;shift+=8){
        const float v=(float)((base>>shift)&255u)*(1.f-weight)+(float)((active>>shift)&255u)*weight;
        out|=((uint32_t)(v+.5f)&255u)<<shift;
    }
    return out;
}
}
