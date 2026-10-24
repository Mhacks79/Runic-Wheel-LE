#pragma once
// Copyright (c) 2026 Mhacks79. Original Runic Wheel LE model.
// Pure, host-testable choice of WHICH owned copy of a FormID the wheel acts on.
// The TESV adapter fills InstanceInfo on the game thread; nothing here touches memory.
//
// A binding's variantId is the SKSE LE inventory itemId of the hovered row:
// CRC32(InventoryEntryData::GenerateName(row), formID & 0x00FFFFFF). For one copy of
// a FormID that is its own extra list, the same hash is computed from a one-list entry.
namespace rw26 {

// SKSE LE HashUtil::CRC32 (reflected 0xEDB88320, seeded with ~start), bit by bit:
// no table, so nothing needs a constructor and the result equals zlib.crc32(s,start).
inline unsigned crc32(const char* s,unsigned start,unsigned maxLength=512){
    unsigned crc=~start;
    if(!s)return ~crc;
    for(unsigned n=0;n<maxLength && s[n];++n){
        crc^=(unsigned char)s[n];
        for(unsigned k=0;k<8;++k)crc=(crc>>1)^(0xEDB88320u & (0u-(crc&1u)));
    }
    return ~crc;
}
inline unsigned item_hash(const char* name,unsigned formId){return crc32(name,formId&0x00FFFFFFu);}

struct InstanceInfo {
    unsigned hash=0;      // item hash of this copy (base hash when nothing custom)
    int count=1;          // ExtraCount, at least one
    bool customized=false;// tempered, enchanted or renamed: not interchangeable
    bool wornRight=false; // ExtraWorn
    bool wornLeft=false;  // ExtraWornLeft
};
enum Pick : unsigned char { PickNone=0, PickPlain, PickList };
struct Choice { Pick pick=PickNone; int index=-1; };

// variant 0 = a binding made before v0.26 (or without SKSE item data): any copy works,
// an ordinary one first. variant == baseHash = the hovered row was an ordinary copy.
inline Choice choose(unsigned variant,unsigned baseHash,int total,const InstanceInfo* lists,int n){
    Choice c{};
    if(total<1 || n<0 || (n && !lists))return c;
    int customized=0;
    for(int i=0;i<n;++i)if(lists[i].customized)customized+=lists[i].count>0?lists[i].count:1;
    const int plain=total-customized;
    if(variant==0 || variant==baseHash){
        if(plain>0){c.pick=PickPlain;return c;}
        for(int i=0;i<n;++i)if(variant!=0 && lists[i].hash==variant){c.pick=PickList;c.index=i;return c;}
        if(variant==0)for(int i=0;i<n;++i)if(lists[i].customized){c.pick=PickList;c.index=i;return c;}
        return c;
    }
    for(int i=0;i<n;++i)if(lists[i].hash==variant){c.pick=PickList;c.index=i;return c;}
    return c; // the exact copy was sold, dropped or renamed: never substitute another one
}

enum Move : unsigned char { MoveRefuse=0, MoveEquip, MoveUnequip };
// Hand items (weapons, scrolls). formInHand: the actor's hand already holds this FormID.
// wornIndex: the list carrying the hand's worn flag, -1 when none was found.
inline Move hand_move(const Choice& c,const InstanceInfo* lists,int n,bool formInHand,int wornIndex){
    if(c.pick==PickNone)return MoveRefuse;
    if(!formInHand)return MoveEquip;
    if(wornIndex<0 || wornIndex>=n)return c.pick==PickPlain?MoveRefuse:MoveEquip;
    if(c.pick==PickList)return wornIndex==c.index?MoveUnequip:MoveEquip;
    return lists[wornIndex].customized?MoveEquip:MoveUnequip;
}
// Armor and torches: worn anywhere. Returns the list to unequip in *wornOut.
inline Move worn_move(const Choice& c,const InstanceInfo* lists,int n,int* wornOut){
    if(wornOut)*wornOut=-1;
    if(c.pick==PickNone)return MoveRefuse;
    if(c.pick==PickList){
        if(c.index<0 || c.index>=n)return MoveRefuse;
        if(lists[c.index].wornRight || lists[c.index].wornLeft){if(wornOut)*wornOut=c.index;return MoveUnequip;}
        return MoveEquip;
    }
    for(int i=0;i<n;++i)if(!lists[i].customized && (lists[i].wornRight || lists[i].wornLeft)){
        if(wornOut)*wornOut=i;return MoveUnequip;
    }
    return MoveEquip;
}

// What may be put on the wheel at all. formType is the TESForm type byte, spellType
// SpellItem::type (0 spell, 2 power, 3 lesser power; abilities and the rest are not
// equipped by anything). Everything accepted here has a native route in perform_action.
inline bool addable(unsigned formType,unsigned spellType){
    switch(formType){
    case 0x29: case 0x1A: case 0x2A: case 0x2E: case 0x1E: case 0x1F: case 0x17: case 0x77: return true;
    case 0x16: return spellType==0 || spellType==2 || spellType==3;
    default: return false;
    }
}
// Only weapons and armor can be tempered, enchanted by the player or renamed.
inline bool keeps_variant(unsigned formType){return formType==0x29 || formType==0x1A;}
// Powers go to the voice slot: EquipSpell picks it itself when handed no slot (0x006EDD00).
inline bool spell_takes_hand(unsigned spellType){return spellType==0;}
}
