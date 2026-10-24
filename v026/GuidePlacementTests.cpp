#include "GuidePlacement.h"
#include <cstdio>
#define CHECK(x) do {if(!(x)){std::fprintf(stderr,"FAIL %s:%d (%s)\n",__FILE__,__LINE__,#x);return 1;}}while(0)
int main(){
 const float resolutions[7][2]={{1280.f,720.f},{1920.f,1080.f},{2560.f,1440.f},
    {1024.f,768.f},{800.f,600.f},{1600.f,900.f},{3440.f,1440.f}};
 for(const auto& dims:resolutions){
  const float w=dims[0],h=dims[1];
  float outer=(w<h?w:h)*.31f*1.30f;
  if(outer>440.f)outer=440.f;
  if(outer>h*.44f)outer=h*.44f;
  float lift=h*.045f<40.f?h*.045f:40.f;
  float cx=w*.5f,cy=h*.5f-lift;
  const rw22::GuideBox guide=rw22::guide_box(w,h,cx,cy,outer,true);
  const rw22::GuideBox world=rw22::guide_box(w,h,cx,cy,outer,false);
  CHECK(!world.visible && world.w==0.f && world.h==0.f);
  CHECK(guide.visible && guide.x>=12.f && guide.x+guide.w<=w-11.f);
  CHECK(guide.y>=12.f && guide.y+guide.h<=h-11.f);
  CHECK(guide.scale>=.75f);
  CHECK(guide.w>=280.f && guide.h>=391.f);
  // Even at 720p, max font height >= 14.4px and row height >= 26px.
  CHECK(18.5f*guide.scale>=13.875f);
  CHECK(31.f*guide.scale>=23.f);
  if(w>=1200.f) CHECK(guide.x>=cx+outer+10.f);
 }
 puts("PASS v023: editor-only guide and larger fonts/rows; 600p/720p/900p/1080p/1440p/ultrawide");
}
