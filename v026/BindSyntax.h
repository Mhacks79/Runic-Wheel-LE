#pragma once
// Runic Wheel LE v0.5. Independent, strictly parsed optional INI binding format.
// Example: 0100ABCD@12345678/W/R, 01002ABC/S/L
// Legacy [Bindings] still accepts raw FormIDs; [BindingsV2] overrides when present.
#include "BindingModel.h"
namespace rw05 {
inline void whitespace(const char*& p){while(*p==' '||*p=='\t')++p;}
inline bool hex_part(const char*& p,unsigned& value){
    whitespace(p);if(p[0]=='0'&&(p[1]=='x'||p[1]=='X'))p+=2;
    unsigned n=0,out=0,d=0;
    while(rw03::hex_digit(*p,d)){
        if(n++>=8)return false;
        out=(out<<4)|d;++p;
    }
    if(!n)return false;
    value=out;return true;
}
inline bool code_kind(char c,unsigned char& out){
    switch(c){
        case 'U':out=Unspecified;return true;
        case 'W':out=Weapon;return true;
        case 'A':out=Armor;return true;
        case 'S':out=Spell;return true;
        case 'H':out=Shout;return true;
        case 'C':out=Consumable;return true;
        case 'N':out=Ammo;return true;
        default:return false;
    }
}
inline bool code_hand(char c,unsigned char& out){
    switch(c){
        case 'A':out=Automatic;return true;
        case 'L':out=Left;return true;
        case 'R':out=Right;return true;
        case 'V':out=Voice;return true;
        default:return false;
    }
}
inline bool parse_one(const char* start, Binding& out){
    if(!start)return false;
    const char* p=start;Binding b{};
    if(!hex_part(p,b.formId) || !b.formId)return false;
    whitespace(p);
    if(*p=='@'){
        ++p;if(!hex_part(p,b.variantId)||!b.variantId)return false;
        whitespace(p);
    }
    if(*p=='/'){
        ++p;whitespace(p);
        if(!code_kind(*p,b.kind))return false;
        ++p;
        whitespace(p);
        if(*p=='/'){
            ++p;whitespace(p);
            if(!code_hand(*p,b.hand))return false;
            ++p;
            whitespace(p);
        }
    }
    whitespace(p);
    if(*p)return false;
    if(!valid_binding(b))return false;
    out=b;return true;
}
inline int parse_slot(const char* line, Slot& slot){
    slot={};if(!line)return 0;
    int rejected=0;
    const char* p=line;
    while(*p){
        char token[48]{};unsigned length=0;bool tooLong=false;
        while(*p && *p!=','){
            if(length+1<sizeof(token))token[length++]=*p;
            else tooLong=true;
            ++p;
        }
        if(*p==',')++p;
        Binding b{};
        if(tooLong||!parse_one(token,b)||!add(slot,b))++rejected;
    }
    return rejected;
}
}
