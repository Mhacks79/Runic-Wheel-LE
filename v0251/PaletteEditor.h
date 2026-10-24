#pragma once
// Original, bounded palette-driven in-wheel editor. No TESV pointers or allocations.
#include "BindSyntax.h"
namespace rw06 {
constexpr unsigned PaletteLimit=12;
struct Palette {rw05::Binding entry[PaletteLimit]{};unsigned char count=0;};
inline bool add_palette(Palette& p,rw05::Binding b){
    if(!rw05::valid_binding(b)||p.count>=PaletteLimit)return false;
    for(unsigned i=0;i<p.count;++i)if(rw05::equal(p.entry[i],b))return false;
    p.entry[p.count++]=b;return true;
}
enum Tool : unsigned char { Add=0, Move=1, Copy=2 };
enum Result : unsigned char { NoChange=0, Added, Removed, SelectedSource, Relocated, Copied, Rejected };
struct Ui {
    unsigned char tool=Add;
    unsigned char paletteIndex=0;
    bool sourcePending=false;
    unsigned char fromWheel=0,fromSector=0,fromIndex=0;
};
inline void cycle_tool(Ui& u){u.tool=(unsigned char)((u.tool+1)%3);u.sourcePending=false;}
inline void cycle_palette(Ui& u,const Palette& p,int direction){
    if(!p.count){u.paletteIndex=0;return;}
    u.paletteIndex=(unsigned char)rw03::wrap((int)u.paletteIndex+direction,p.count);
}
inline Result click(Ui& u,rw05::Editor& e,rw05::State& s,
                    const Palette& p,unsigned wheel,unsigned sector,bool secondary){
    if(wheel>=rw05::Wheels||sector>=rw05::Sectors)return Rejected;
    auto& slot=s.slot[wheel][sector];
    const unsigned index=s.cursor[wheel][sector];
    if(secondary){
        u.sourcePending=false;
        return rw05::erase(e,s,wheel,sector,(int)index)?Removed:Rejected;
    }
    if(u.tool==Add){
        if(!p.count||u.paletteIndex>=p.count)return Rejected;
        if(!rw05::insert(e,s,wheel,sector,p.entry[u.paletteIndex]))return Rejected;
        s.cursor[wheel][sector]=(unsigned char)(s.slot[wheel][sector].count-1);
        return Added;
    }
    if(!u.sourcePending){
        if(index>=slot.count)return Rejected;
        u.sourcePending=true;u.fromWheel=(unsigned char)wheel;u.fromSector=(unsigned char)sector;u.fromIndex=(unsigned char)index;
        return SelectedSource;
    }
    const bool copy=u.tool==Copy;
    const bool changed=rw05::relocate(e,s,u.fromWheel,u.fromSector,u.fromIndex,wheel,sector,copy);
    // Keep the source selected after failed target, allowing retry without reselecting.
    if(changed)u.sourcePending=false;
    return changed?(copy?Copied:Relocated):Rejected;
}
}
