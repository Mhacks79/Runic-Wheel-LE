#pragma once
// Pure, testable UI geometry and input ownership. No SKSE/Win32 dependency.
namespace rw25 {
struct Rect {float x,y,w,h;};
struct Offset {float x,y;};
inline float clamp(float v,float lo,float hi){return v<lo?lo:(v>hi?hi:v);}
inline bool contains(Rect r,float x,float y){return x>=r.x&&y>=r.y&&x<r.x+r.w&&y<r.y+r.h;}
inline Offset restrict_offset(float screenW,float screenH,float baseX,float baseY,float windowW,float windowH,float x,float y){
    const float pad=12.f;
    return {clamp(x,pad-baseX,screenW-pad-windowW-baseX),
            clamp(y,pad-baseY,screenH-pad-windowH-baseY)};
}
// Internal bitmap font's 64px cells have glyph alpha typically within y=8..43,
// and a reference cap-height of 46px. Centre actual ink, not the padded cell.
inline float centered_text_y(float boxY,float boxH,float fontPx){
    return boxY+(boxH-(35.f/46.f)*fontPx)*.5f-(8.f/46.f)*fontPx;
}
inline bool close_on_directinput(unsigned device,unsigned scan,bool down){
    return down && device==0u && (scan==0x0fu || scan==0x01u);
}
inline bool skyrim_tab(unsigned message,unsigned key){
    return (message==0x100u || message==0x104u) && key==0x09u;
}
inline bool lost_focus(unsigned message,unsigned wp){
    return message==0x0008u || message==0x001fu ||
           (message==0x001cu && wp==0u);
}
}
