#pragma once
// Copyright (c) 2026 Mhacks79. Original Runic Wheel LE model.
// A variantId is the SKSE inventory display-name CRC32 (itemId), NOT ExtraUniqueID.
// Multiple physically distinct items can still share the same display hash.
#include "CatalogSave.h"
namespace rw05 {
constexpr unsigned Wheels=rw03::Wheels, Sectors=rw03::Slots, Capacity=rw03::Entries;
enum Kind : unsigned char { Unspecified=0, Weapon=1, Armor=2, Spell=3, Shout=4, Consumable=5, Ammo=6, Torch=7 };
enum Hand : unsigned char { Automatic=0, Left=1, Right=2, Voice=3 };
struct Binding {
    unsigned formId=0;
    unsigned variantId=0; // 0=base type; nonzero=CRC32(name, formId & 0x00FFFFFF), SKSE LE
    unsigned char kind=Unspecified;
    unsigned char hand=Automatic;
    unsigned char flags=0;
    unsigned char reserved=0;
};
struct Slot { Binding item[Capacity]{}; unsigned char count=0; };
struct State {
    Slot slot[Wheels][Sectors]{};
    unsigned char cursor[Wheels][Sectors]{};
    unsigned char activeWheel=0;
};
inline bool equal(const Binding& a,const Binding& b){return a.formId==b.formId && a.variantId==b.variantId;}
inline bool valid_binding(const Binding& b){
    if(!b.formId || b.kind>Torch || b.hand>Voice || b.flags || b.reserved)return false;
    if(b.kind==Spell && b.variantId)return false;
    if(b.kind==Shout && (b.variantId || (b.hand!=Automatic && b.hand!=Voice)))return false;
    if(b.kind==Armor || b.kind==Consumable || b.kind==Ammo || b.kind==Torch){if(b.hand!=Automatic)return false;}
    if(b.kind==Weapon || b.kind==Spell){if(b.hand==Voice)return false;}
    if(b.kind==Unspecified && b.hand==Voice)return false;
    return true;
}
inline bool add(Slot& slot,Binding b){
    if(!valid_binding(b) || slot.count>=Capacity)return false;
    for(unsigned i=0;i<slot.count;++i)if(equal(slot.item[i],b))return false;
    slot.item[slot.count++]=b;return true;
}
inline bool remove(Slot& s,int idx){
    if(idx<0 || idx>=s.count)return false;
    for(int i=idx+1;i<(int)Capacity && i<s.count;++i)s.item[i-1]=s.item[i];
    s.item[--s.count]={};return true;
}
inline bool validate(const State& s){
    if(s.activeWheel>=Wheels)return false;
    for(unsigned w=0;w<Wheels;++w)for(unsigned p=0;p<Sectors;++p){
        const Slot& slot=s.slot[w][p];
        if(slot.count>Capacity || (slot.count==0 ? s.cursor[w][p]!=0 : s.cursor[w][p]>=slot.count))return false;
        for(unsigned i=0;i<Capacity;++i){
            const Binding& b=slot.item[i];
            if(i>=slot.count){if(b.formId||b.variantId||b.kind||b.hand||b.flags||b.reserved)return false;continue;}
            if(!valid_binding(b))return false;
            for(unsigned j=0;j<i;++j)if(equal(slot.item[j],b))return false;
        }
    }
    return true;
}
inline Binding current(const State& s,unsigned wheel,unsigned sector){
    if(wheel>=Wheels||sector>=Sectors)return {};
    const Slot& slot=s.slot[wheel][sector];
    if(!slot.count || slot.count>Capacity)return {};
    unsigned n=s.cursor[wheel][sector];return n<slot.count?slot.item[n]:Binding{};
}
inline State migrate(const rw04::State& old){
    State next{};next.activeWheel=old.activeWheel;
    for(unsigned w=0;w<Wheels;++w)for(unsigned p=0;p<Sectors;++p){
        const auto& oldSlot=old.catalog.slots[w][p];
        for(unsigned i=0;i<oldSlot.count&&i<Capacity;++i){
            Binding b{};b.formId=oldSlot.form[i]; // type/instance unknown: fail closed on use
            (void)add(next.slot[w][p],b);
        }
        next.cursor[w][p]=next.slot[w][p].count ? (old.cursor[w][p]<next.slot[w][p].count?old.cursor[w][p]:0) : 0;
    }
    return next;
}
struct Editor {
    State undo[8]{},redo[8]{};
    unsigned char undoCount=0,redoCount=0;
};
inline void push(State (&stack)[8],unsigned char& n,const State& before){
    if(n==8){for(unsigned i=1;i<8;++i)stack[i-1]=stack[i];--n;}
    stack[n++]=before;
}
inline bool commit(Editor& e,State& target,const State& candidate){
    if(!validate(candidate))return false;
    push(e.undo,e.undoCount,target);e.redoCount=0;target=candidate;return true;
}
inline bool insert(Editor& e,State& target,unsigned wheel,unsigned sector,Binding b){
    if(wheel>=Wheels||sector>=Sectors)return false;
    State next=target;
    if(!add(next.slot[wheel][sector],b))return false;
    return commit(e,target,next);
}
inline bool erase(Editor& e,State& target,unsigned wheel,unsigned sector,int index){
    if(wheel>=Wheels||sector>=Sectors)return false;
    State next=target;
    if(!remove(next.slot[wheel][sector],index))return false;
    const unsigned n=next.slot[wheel][sector].count;
    next.cursor[wheel][sector]=n?static_cast<unsigned char>(index<(int)n?index:n-1):0;
    return commit(e,target,next);
}
inline bool relocate(Editor& e,State& target,unsigned srcW,unsigned srcS,int srcI,unsigned dstW,unsigned dstS,bool copy){
    if(srcW>=Wheels||dstW>=Wheels||srcS>=Sectors||dstS>=Sectors)return false;
    if(!copy && srcW==dstW && srcS==dstS)return false;
    const auto& src=target.slot[srcW][srcS];
    if(srcI<0||srcI>=src.count)return false;
    State next=target;
    if(!add(next.slot[dstW][dstS],src.item[srcI]))return false;
    if(!copy){
        if(!remove(next.slot[srcW][srcS],srcI))return false;
        unsigned n=next.slot[srcW][srcS].count;
        next.cursor[srcW][srcS]=n?static_cast<unsigned char>(srcI<(int)n?srcI:n-1):0;
    }
    next.cursor[dstW][dstS]=next.slot[dstW][dstS].count-1;
    return commit(e,target,next);
}
inline bool history(Editor& e,State& target,bool undo){
    unsigned char& n=undo?e.undoCount:e.redoCount;
    if(!n)return false;
    if(undo)push(e.redo,e.redoCount,target);else push(e.undo,e.undoCount,target);
    target=(undo?e.undo:e.redo)[--n];return validate(target);
}
}
