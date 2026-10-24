#pragma once
// Runic Wheel LE: independent UI behavior, with no game-engine assumptions.
namespace rw07 {
inline bool menu_recent(unsigned now,unsigned last,unsigned threshold=600u){
    return last!=0 && (unsigned)(now-last)<threshold;
}
inline bool should_slow(bool opened,bool menuVisible){return opened && !menuVisible;}
enum Action { None, AddHovered, RemoveSelected, ApplySelected };
inline Action click(bool wheelVisible,bool inInventory,bool secondary,unsigned formId){
    if(!wheelVisible)return None;
    if(inInventory)return secondary?RemoveSelected:(formId?AddHovered:None);
    return ApplySelected;
}
}
