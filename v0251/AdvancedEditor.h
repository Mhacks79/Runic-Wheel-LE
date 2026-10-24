#pragma once
// Runic Wheel LE: purely data-driven inventory editor operations. No Bethesda calls.
// Every operation is atomic, validated, undoable, and respects 4 items/sector.
#include "BindingModel.h"
namespace rw21 {
enum Result : unsigned char {Invalid=0, Armed, Applied, Cancelled, Nothing, TargetFull, Duplicate, Stale};
enum Mode : unsigned char {None=0, MoveItem, CopyItem, SwapSector, CloneSector};
enum Gesture : unsigned char {Plain=0, Shift=1, Control=2, Alt=4, ShiftControl=3, ShiftAlt=5};
struct Modifiers {
    unsigned char pressed=0;
    bool update(unsigned scan,bool down){
        unsigned bit=0;
        switch(scan){
         case 0x2A: bit=1;break; // left shift
         case 0x36: bit=2;break; // right shift
         case 0x1D: bit=4;break; // left ctrl
         case 0x9D: bit=8;break; // right ctrl
         case 0x38: bit=16;break; // left alt
         case 0xB8: bit=32;break; // right alt
         default:return false;
        }
        if(down)pressed=(unsigned char)(pressed|bit);else pressed=(unsigned char)(pressed&~bit);
        return true;
    }
    unsigned gesture()const {
       const unsigned shift=(pressed&3u)?1u:0u,ctrl=(pressed&12u)?2u:0u,alt=(pressed&48u)?4u:0u;
       return shift|ctrl|alt;
    }
    void reset(){pressed=0;}
};
struct Pending {
    Mode mode=None;
    unsigned char wheel=0, sector=0, index=0;
    rw05::Binding expected{};
};
inline void cancel(Pending& p){p={};}
inline Result arm(Pending& p,const rw05::State& s,Mode mode,unsigned w,unsigned sector){
    if(mode==None || w>=rw05::Wheels || sector>=rw05::Sectors)return Invalid;
    const rw05::Slot& slot=s.slot[w][sector];
    if(!slot.count || slot.count>rw05::Capacity)return Nothing;
    if(p.mode==mode && p.wheel==w && p.sector==sector){cancel(p);return Cancelled;}
    p={};p.mode=mode;p.wheel=(unsigned char)w;p.sector=(unsigned char)sector;
    p.index=s.cursor[w][sector];
    if(p.index>=slot.count)return Stale;
    p.expected=slot.item[p.index];return Armed;
}
inline Result paste(Pending& p,rw05::Editor& history,rw05::State& s,unsigned w,unsigned sector){
    if(p.mode==None || w>=rw05::Wheels || sector>=rw05::Sectors)return Invalid;
    if(p.wheel==w && p.sector==sector)return Invalid;
    const rw05::Slot& src=s.slot[p.wheel][p.sector];
    if(p.index>=src.count || !rw05::equal(src.item[p.index],p.expected)){
        cancel(p);return Stale;
    }
    rw05::State proposed=s;
    if(p.mode==MoveItem || p.mode==CopyItem){
        const rw05::Slot& dst=proposed.slot[w][sector];
        if(dst.count>=rw05::Capacity)return TargetFull;
        for(unsigned i=0;i<dst.count;++i)if(rw05::equal(dst.item[i],p.expected))return Duplicate;
        if(!rw05::add(proposed.slot[w][sector],p.expected))return Invalid;
        proposed.cursor[w][sector]=(unsigned char)(proposed.slot[w][sector].count-1);
        if(p.mode==MoveItem){
            if(!rw05::remove(proposed.slot[p.wheel][p.sector],p.index))return Invalid;
            const unsigned count=proposed.slot[p.wheel][p.sector].count;
            proposed.cursor[p.wheel][p.sector]=(unsigned char)(count ? (p.index<count?p.index:count-1) : 0);
        }
    }else if(p.mode==SwapSector){
        const rw05::Slot dst=proposed.slot[w][sector];
        const unsigned char dstCursor=proposed.cursor[w][sector];
        proposed.slot[w][sector]=proposed.slot[p.wheel][p.sector];
        proposed.cursor[w][sector]=proposed.cursor[p.wheel][p.sector];
        proposed.slot[p.wheel][p.sector]=dst;
        proposed.cursor[p.wheel][p.sector]=dstCursor;
    }else if(p.mode==CloneSector){
        if(proposed.slot[w][sector].count)return TargetFull; // explicit no-overwrite rule
        proposed.slot[w][sector]=proposed.slot[p.wheel][p.sector];
        proposed.cursor[w][sector]=proposed.cursor[p.wheel][p.sector];
    }else return Invalid;
    if(!rw05::commit(history,s,proposed))return Invalid;
    cancel(p);return Applied;
}
inline bool clear_stack(rw05::Editor& history,rw05::State& s,unsigned w,unsigned sector){
    if(w>=rw05::Wheels || sector>=rw05::Sectors || s.slot[w][sector].count==0)return false;
    rw05::State proposed=s;
    proposed.slot[w][sector]={};proposed.cursor[w][sector]=0;
    return rw05::commit(history,s,proposed);
}
inline bool rotate_entry(rw05::Editor& history,rw05::State& s,unsigned w,unsigned sector,int direction){
    if(w>=rw05::Wheels || sector>=rw05::Sectors || !direction)return false;
    rw05::State proposed=s;
    auto& slot=proposed.slot[w][sector];
    if(slot.count<2 || slot.count>rw05::Capacity)return false;
    const unsigned current=proposed.cursor[w][sector];
    if(current>=slot.count)return false;
    const unsigned next=(current+ (direction<0?slot.count-1:1))%slot.count;
    const rw05::Binding tmp=slot.item[next];slot.item[next]=slot.item[current];slot.item[current]=tmp;
    proposed.cursor[w][sector]=(unsigned char)next;
    return rw05::commit(history,s,proposed);
}
inline bool clone_wheel(rw05::Editor& history,rw05::State& s,unsigned source,unsigned target){
    if(source>=rw05::Wheels || target>=rw05::Wheels || source==target)return false;
    unsigned occupied=0;
    for(unsigned i=0;i<rw05::Sectors;++i){
        if(s.slot[target][i].count)return false; // no overwrite
        if(s.slot[source][i].count)++occupied;
    }
    if(!occupied)return false;
    rw05::State next=s;
    for(unsigned i=0;i<rw05::Sectors;++i){
        next.slot[target][i]=s.slot[source][i];
        next.cursor[target][i]=s.cursor[source][i];
    }
    return rw05::commit(history,s,next);
}
inline bool swap_wheels(rw05::Editor& history,rw05::State& s,unsigned a,unsigned b){
    if(a>=rw05::Wheels || b>=rw05::Wheels || a==b)return false;
    rw05::State next=s;
    for(unsigned i=0;i<rw05::Sectors;++i){
        const rw05::Slot saved=next.slot[a][i];
        next.slot[a][i]=next.slot[b][i];next.slot[b][i]=saved;
        const unsigned char savedCursor=next.cursor[a][i];
        next.cursor[a][i]=next.cursor[b][i];next.cursor[b][i]=savedCursor;
    }
    return rw05::commit(history,s,next);
}
inline bool clear_wheel(rw05::Editor& history,rw05::State& s,unsigned w){
    if(w>=rw05::Wheels)return false;
    unsigned occupied=0;for(unsigned i=0;i<rw05::Sectors;++i)occupied+=s.slot[w][i].count>0;
    if(!occupied)return false;
    rw05::State next=s;
    for(unsigned i=0;i<rw05::Sectors;++i){next.slot[w][i]={};next.cursor[w][i]=0;}
    return rw05::commit(history,s,next);
}
inline bool has_items(const rw05::State& s,unsigned w,unsigned sector){
    return w<rw05::Wheels && sector<rw05::Sectors && s.slot[w][sector].count>0 && s.slot[w][sector].count<=rw05::Capacity;
}
inline unsigned count_entries(const rw05::State& s,unsigned w){
    if(w>=rw05::Wheels)return 0;
    unsigned n=0;for(unsigned i=0;i<rw05::Sectors;++i)n+=s.slot[w][i].count;
    return n;
}
inline unsigned count_occupied(const rw05::State& s,unsigned w){
    if(w>=rw05::Wheels)return 0;
    unsigned n=0;for(unsigned i=0;i<rw05::Sectors;++i)n+=s.slot[w][i].count!=0;
    return n;
}
}
