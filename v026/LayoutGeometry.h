#pragma once
// Geometry-only layout. LE co-save remains the existing 3x8x4 fixed model.
// Lower displayed count hides unused sectors/pages without deleting entries.
namespace rwl24 {
struct Point {float x,y;};
static constexpr Point edges[5][9]={
  { // 4 sectors
    {-0.707106781f,-0.707106781f},
    {0.707106781f,-0.707106781f},
    {0.707106781f,0.707106781f},
    {-0.707106781f,0.707106781f},
    {-0.707106781f,-0.707106781f},
    {0.f,0.f},
    {0.f,0.f},
    {0.f,0.f},
    {0.f,0.f},
  },
  { // 5 sectors
    {-0.587785252f,-0.809016994f},
    {0.587785252f,-0.809016994f},
    {0.951056516f,0.309016994f},
    {0.000000000f,1.000000000f},
    {-0.951056516f,0.309016994f},
    {-0.587785252f,-0.809016994f},
    {0.f,0.f},
    {0.f,0.f},
    {0.f,0.f},
  },
  { // 6 sectors
    {-0.500000000f,-0.866025404f},
    {0.500000000f,-0.866025404f},
    {1.000000000f,0.000000000f},
    {0.500000000f,0.866025404f},
    {-0.500000000f,0.866025404f},
    {-1.000000000f,0.000000000f},
    {-0.500000000f,-0.866025404f},
    {0.f,0.f},
    {0.f,0.f},
  },
  { // 7 sectors
    {-0.433883739f,-0.900968868f},
    {0.433883739f,-0.900968868f},
    {0.974927912f,-0.222520934f},
    {0.781831482f,0.623489802f},
    {0.000000000f,1.000000000f},
    {-0.781831482f,0.623489802f},
    {-0.974927912f,-0.222520934f},
    {-0.433883739f,-0.900968868f},
    {0.f,0.f},
  },
  { // 8 sectors
    {-0.382683432f,-0.923879533f},
    {0.382683432f,-0.923879533f},
    {0.923879533f,-0.382683432f},
    {0.923879533f,0.382683432f},
    {0.382683432f,0.923879533f},
    {-0.382683432f,0.923879533f},
    {-0.923879533f,0.382683432f},
    {-0.923879533f,-0.382683432f},
    {-0.382683432f,-0.923879533f},
  },
};
inline int count(int n){return n<4?4:n>8?8:n;}
inline Point edge(int n,int i){n=count(n);return edges[n-4][i<0?0:i>n?n:i];}
inline Point center(int n,int i){n=count(n);if(i<0)i=0;if(i>=n)i=n-1;
    Point a=edge(n,i),b=edge(n,i+1);const float dx=a.x+b.x,dy=a.y+b.y;
    // normalized midpoint, via precomputed inverse cosines for n=4..8
    static constexpr float factor[]={0.707106781f,0.618033989f,0.577350269f,0.554958132f,0.541196100f};
    return {dx*factor[n-4],dy*factor[n-4]};
}
inline int select(int n,int x,int y,int deadzone){
    const long r2=(long)x*x+(long)y*y;
    if(r2<(long)deadzone*deadzone)return -1;
    n=count(n);int best=0;float bestDot=-1.e10f;
    for(int i=0;i<n;++i){Point p=center(n,i);float dot=p.x*(float)x+p.y*(float)y;
        if(dot>bestDot){bestDot=dot;best=i;}}
    return best;
}
inline int visible_page(int active,int pages){return active<0?0:active>=pages?pages-1:active;}
inline int wrap_page(int current,int delta,int pages){if(pages<1)pages=1;if(pages>3)pages=3;
 int r=(current+delta)%pages;return r<0?r+pages:r;}
}
