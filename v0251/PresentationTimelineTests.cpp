#include "PresentationTimeline.h"
#include <cassert>
#include <cmath>
int main(){
 using namespace rw16;
 auto eq=[](float x,float y){return std::fabs(x-y)<0.00001f;};
 // Entry and exit do not depend on frame rate, and do not affect interaction.
 assert(eq(frame(true,false,true,false,100,100,0,160,110).opacity,0.f));
 assert(frame(true,false,true,false,260,100,0,160,110).opacity==1.f);
 assert(frame(true,false,true,false,260,100,0,160,110).scale==1.f);
 assert(frame(true,false,true,true,180,100,0,160,110).scale==1.f);
 assert(frame(true,false,false,false,100,100,0,160,110).opacity==1.f);
 assert(frame(false,true,true,false,110,0,100,160,110).draw);
 assert(!frame(false,true,true,false,210,0,100,160,110).draw);
 assert(!frame(false,false,true,false,101,0,100,160,110).draw);
 assert(!frame(false,true,false,false,110,0,100,160,110).draw);
 // The 32-bit tick counter wraps approximately every 49.7 days.
 assert(elapsed(9u,0xfffffff6u)==19u);
 assert(eq(progress(9u,0xfffffff6u,19),1.f));
 assert(frame(true,false,true,false,9u,0xfffffff6u,0,19,90).opacity==1.f);
 // Sector 7->0 and wheel 2->0 are unmodified *indices*; only colors interpolate.
 assert(eq(selected_blend(200,100,100,true,false),1.f));
 assert(eq(selected_blend(100,100,100,true,false),0.f));
 assert(eq(selected_blend(100,100,100,true,true),1.f));
 assert(feedback_alpha(100,100,900)==1.f);
 assert(feedback_alpha(1000,100,900)==0.f);
 assert(feedback_alpha(900,100,900)>0.f && feedback_alpha(900,100,900)<1.f);
 assert(with_alpha(0xffaabbccu,.5f)==0x80aabbccu);
 assert(lerp_color(0x00202020u,0xffaabbccu,0.f)==0x00202020u);
 assert(lerp_color(0x00202020u,0xffaabbccu,1.f)==0xffaabbccu);
 assert(lerp_color(0x00202020u,0xffaabbccu,.5f)!=0x00202020u);
}
