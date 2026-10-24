#pragma once
// Pure, host-testable gate. The TESV adapter applies further pointer/ABI/thread checks.
#include "BindingModel.h"
namespace rw11 {
enum Result : unsigned char {Refuse=0, Equip, Unequip, Consume, EquipSpell};
struct Facts {
    bool focused=false;
    bool inGame=false;
    bool actorReady=false;
    bool owned=false;
    bool ambiguous=false;
    bool learned=false;
    bool worn=false;
    rw05::Kind actualKind=rw05::Unspecified;
    int availableCount=0;
};
inline Result decide(const rw05::Binding& entry,const Facts& f){
    if(!rw05::valid_binding(entry) || !f.focused || !f.inGame || !f.actorReady)return Refuse;
    if(entry.kind && entry.kind!=(unsigned)f.actualKind)return Refuse;
    if(entry.variantId || f.actualKind==rw05::Unspecified || f.actualKind==rw05::Shout)return Refuse;
    if(f.actualKind==rw05::Spell)return f.learned?EquipSpell:Refuse;
    if(!f.owned || f.availableCount<1 || f.ambiguous)return Refuse;
    if(f.actualKind==rw05::Consumable)return Consume;
    if(f.actualKind==rw05::Armor || f.actualKind==rw05::Weapon || f.actualKind==rw05::Ammo || f.actualKind==rw05::Torch)
        return f.worn?Unequip:Equip;
    return Refuse;
}
}
