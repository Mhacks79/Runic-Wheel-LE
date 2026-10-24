#include "KhaosWindowPolicy.h"
#include <cassert>
#include <cmath>
int main(){
    using namespace rw25;
    Rect window{320,120,960,720};
    assert(contains(window,340,150)&&!contains(window,310,150));
    // Caption can be dragged and the window must never disappear offscreen.
    Offset a=restrict_offset(1920,1080,320,120,960,720,-9999,-9999);
    assert(std::abs((320+a.x)-12.f)<.01f && std::abs((120+a.y)-12.f)<.01f);
    Offset b=restrict_offset(1920,1080,320,120,960,720,9999,9999);
    assert(std::abs((320+b.x+960.f)-(1920.f-12.f))<.01f);
    assert(std::abs((120+b.y+720.f)-(1080.f-12.f))<.01f);
    // Ink centering rather than centering the 64-pixel padded glyph tile.
    const float y=centered_text_y(200.f,36.f,18.f);
    const float inkTop=y+18.f*8.f/46.f;
    const float inkBottom=y+18.f*43.f/46.f;
    assert(std::abs(((inkTop+inkBottom)*.5f)-218.f)<.01f);
    // Tab must relinquish UI ownership; Escape closes without opening menu.
    assert(skyrim_tab(0x100,9)&&!skyrim_tab(0x100,0x0d));
    assert(close_on_directinput(0,15,true));
    assert(close_on_directinput(0,1,true));
    assert(!close_on_directinput(1,15,true));
    assert(!close_on_directinput(0,15,false));
    assert(lost_focus(0x8,0)&&lost_focus(0x1c,0)&&!lost_focus(0x1c,1));
    assert(!lost_focus(0x100,9) && !lost_focus(0x215,0));
}
