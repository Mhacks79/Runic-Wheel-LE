#pragma once
// Small original, platform-independent state machine, deliberately CRT-free.
namespace rw02 {
struct Activation { unsigned int pressedAt=0; bool pressed=false, openedHere=false, visible=false; };
inline void reset(Activation& a) { a.pressed=false; a.openedHere=false; a.visible=false; a.pressedAt=0; }
inline bool button(Activation& a, bool down, unsigned int now, unsigned int holdMs) {
    if (down && !a.pressed) {
        a.pressed=true;
        a.pressedAt=now;
        a.openedHere=!a.visible;
        a.visible=!a.visible; // tap on an already open wheel closes immediately
    } else if (!down && a.pressed) {
        a.pressed=false;
        if(a.openedHere && (now-a.pressedAt)>=holdMs) a.visible=false;
        a.openedHere=false;
    }
    return a.visible;
}
inline int clamp(int n,int lo,int hi) { return n<lo?lo:(n>hi?hi:n); }
// Sectors clockwise starting at 12 o'clock, matching the DX9 wheel mesh.
inline int sector(int x,int y,int deadzone) {
    if(x*x+y*y<deadzone*deadzone) return -1;
    const int ux[8]={0,7071,10000,7071,0,-7071,-10000,-7071};
    const int uy[8]={-10000,-7071,0,7071,10000,7071,0,-7071};
    int best=0; long score=-0x7fffffffL;
    for(int i=0;i<8;++i) {long dot=(long)x*ux[i]+(long)y*uy[i]; if(dot>score){score=dot;best=i;}}
    return best;
}
inline int step(int current,int delta,int slots) {
    if(slots<=0)return 0;
    int value=(current+delta)%slots;
    return value<0?value+slots:value;
}
}
