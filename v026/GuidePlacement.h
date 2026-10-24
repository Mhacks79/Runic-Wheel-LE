#pragma once
// Offline-testable responsive guide placement; no TESV/Win32/D3D9 dependencies.
namespace rw22 {
struct GuideBox {float x,y,w,h,scale;bool compact,visible;};
inline GuideBox guide_box(float width,float height,float cx,float cy,float outer,bool editor){
    // Up to 2560x1440: make the 13 editor commands genuinely readable at the
    // user's native resolution; compact on 720p, never scale fonts below 14px.
    float scale=width>=2200.f && height>=1200.f?1.28f:
        width>=1600.f && height>=900.f?1.15f:
        width>=1200.f && height>=680.f?.96f:.86f;
    // There is no world guide in v0.23; editor parameter is retained only
    // for pure layout compatibility tests.
    if(!editor)return {0,0,0,0,scale,true,false};
    const float h=566.f*scale;
    const float margin=12.f;
    const float right=cx+outer+18.f;
    const float freeRight=width-right-margin;
    // Keep the guide to the RIGHT on 720p too by adapting panel width,
    // not shrinking its typography again. Only very narrow viewports overlap.
    float w=498.f*scale;
    if(freeRight>=280.f && w>freeRight)w=freeRight;
    if(freeRight<280.f)w=280.f;
    float x=right;
    if(x+w>width-margin)x=width-margin-w;
    if(x<margin)x=margin;
    float y=cy-outer*.43f;
    if(y+h>height-margin)y=height-margin-h;
    if(y<margin)y=margin;
    return {x,y,w,h,scale,scale<1.f,width>=w+2.f*margin && height>=h+2.f*margin};
}
}
