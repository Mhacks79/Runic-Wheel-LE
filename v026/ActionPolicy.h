#pragma once
#include "BindingModel.h"
// Plans a gameplay action but NEVER invokes an engine function from this module.
// The engine adapter must additionally prove actor, inventory, calling convention and thread.
namespace rw05 {
enum Operation : unsigned char { Reject, EquipObject, EquipMagic, EquipVoice, Consume };
enum Reason : unsigned char { Allowed, Empty, UnknownType, MissingActor, MissingOwnership, MissingVariant, BlockedContext, InvalidHand, NotLearned };
struct Context {
    bool focused=false,gameplay=false,actorReady=false,owned=false;
    bool exactVariantFound=false,learned=false;
    Kind resolvedKind=Unspecified;
};
struct Decision { Operation operation=Reject; Reason reason=Empty; Binding binding{}; };
inline Decision plan(Binding b,const Context& c){
    Decision d{};d.binding=b;
    if(!b.formId){d.reason=Empty;return d;}
    if(!valid_binding(b)){d.reason=InvalidHand;return d;}
    if(!c.focused||!c.gameplay){d.reason=BlockedContext;return d;}
    if(!c.actorReady){d.reason=MissingActor;return d;}
    Kind kind=b.kind==Unspecified?c.resolvedKind:static_cast<Kind>(b.kind);
    if(kind==Unspecified || kind>Ammo){d.reason=UnknownType;return d;}
    if(kind!=c.resolvedKind){d.reason=UnknownType;return d;} // never trust stale INI type
    if(kind==Spell||kind==Shout){
        if(!c.learned){d.reason=NotLearned;return d;}
        if((kind==Spell && b.hand==Voice)||(kind==Shout && b.hand!=Automatic&&b.hand!=Voice)){
            d.reason=InvalidHand;return d;
        }
        d.operation=kind==Spell?EquipMagic:EquipVoice;
    }else{
        if(!c.owned){d.reason=MissingOwnership;return d;}
        if(b.variantId && !c.exactVariantFound){d.reason=MissingVariant;return d;}
        if(kind==Weapon && b.hand==Voice){d.reason=InvalidHand;return d;}
        if((kind==Armor||kind==Consumable||kind==Ammo) && b.hand!=Automatic){d.reason=InvalidHand;return d;}
        if(kind==Consumable)d.operation=Consume;
        else d.operation=EquipObject;
    }
    d.reason=Allowed;return d;
}
// One-shot queue: no pointers to engine objects persist; discard stale commands across load/menu transitions.
struct Pending { Binding binding{}; unsigned generation=0; bool available=false; };
inline void reset(Pending& p,unsigned generation){p={};p.generation=generation;}
inline bool enqueue(Pending& p,Binding b,unsigned generation){
    if(!valid_binding(b)||p.available||p.generation!=generation)return false;
    p.binding=b;p.available=true;return true;
}
inline bool consume(Pending& p,unsigned generation,Binding& result){
    if(!p.available||p.generation!=generation)return false;
    result=p.binding;p.available=false;return true;
}
}
