#pragma once
// Pure, host-testable gate. The TESV adapter applies further pointer/ABI/thread checks.
#include "BindingModel.h"
namespace rw11 {
enum Result : unsigned char {Refuse=0, Equip, Unequip, Consume, EquipSpell, EquipShout};
struct Facts {
    bool focused=false;
    bool inGame=false;
    bool actorReady=false;
    bool owned=false;
    bool instanceMissing=false; // the bound copy (or any copy) could not be resolved
    bool learned=false;         // spell or shout known, shout with an unlocked first word
    bool worn=false;
    rw05::Kind actualKind=rw05::Unspecified;
    int availableCount=0;
    bool scroll=false;          // SCRL 0x17, stored as Unspecified for RWL2 compatibility
};
inline Result decide(const rw05::Binding& entry,const Facts& f){
    if(!rw05::valid_binding(entry) || !f.focused || !f.inGame || !f.actorReady)return Refuse;
    if(entry.kind && entry.kind!=(unsigned)f.actualKind)return Refuse;
    if(f.actualKind==rw05::Spell)return (!entry.variantId && f.learned)?EquipSpell:Refuse;
    if(f.actualKind==rw05::Shout)return (!entry.variantId && f.learned)?EquipShout:Refuse;
    const bool scroll=f.actualKind==rw05::Unspecified && f.scroll;
    if(f.actualKind==rw05::Unspecified && !scroll)return Refuse;
    if(!f.owned || f.availableCount<1 || f.instanceMissing)return Refuse;
    if(f.actualKind==rw05::Consumable)return Consume;
    if(scroll || f.actualKind==rw05::Armor || f.actualKind==rw05::Weapon ||
       f.actualKind==rw05::Ammo || f.actualKind==rw05::Torch)
        return f.worn?Unequip:Equip;
    return Refuse;
}
}
