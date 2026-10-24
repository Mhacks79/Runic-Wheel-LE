#pragma once
#include "BindingModel.h"
namespace rw05 {
// 'RWL2' packet, explicit endianness, bounded, CRC32 of whole packet except checksum.
constexpr unsigned Header=16, EntryBytes=12, SectorBytes=2+Capacity*EntryBytes;
constexpr unsigned PacketBytes=Header+Wheels*Sectors*SectorBytes;
static_assert(PacketBytes==1216,"Update save version after any format change");
using Resolver=rw04::Resolver;
inline void encode(const State& s,unsigned char (&out)[PacketBytes]){
    for(unsigned i=0;i<PacketBytes;++i)out[i]=0;
    if(!validate(s))return;
    out[0]='R';out[1]='W';out[2]='L';out[3]='2';
    out[4]=2;out[5]=Wheels;out[6]=Sectors;out[7]=Capacity;out[8]=s.activeWheel;
    unsigned off=Header;
    for(unsigned w=0;w<Wheels;++w)for(unsigned p=0;p<Sectors;++p){
        const Slot& slot=s.slot[w][p];out[off++]=slot.count;out[off++]=s.cursor[w][p];
        for(unsigned i=0;i<Capacity;++i){
            const Binding& b=slot.item[i];
            rw04::put32(out+off,b.formId);off+=4;
            rw04::put32(out+off,b.variantId);off+=4;
            out[off++]=b.kind;out[off++]=b.hand;out[off++]=b.flags;out[off++]=b.reserved;
        }
    }
    unsigned hash=rw04::crc32(out,12);
    hash=~rw04::crc32(out+Header,PacketBytes-Header,hash);
    rw04::put32(out+12,hash);
}
inline bool decode(const unsigned char* p,unsigned n,State& target,Resolver resolve,void* ctx,unsigned* dropped=nullptr){
    if(!p||n!=PacketBytes||!resolve)return false;
    if(p[0]!='R'||p[1]!='W'||p[2]!='L'||p[3]!='2'||p[4]!=2||
       p[5]!=Wheels||p[6]!=Sectors||p[7]!=Capacity||p[8]>=Wheels||p[9]||p[10]||p[11])return false;
    unsigned hash=rw04::crc32(p,12);
    hash=~rw04::crc32(p+Header,PacketBytes-Header,hash);
    if(hash!=rw04::get32(p+12))return false;
    State next{};next.activeWheel=p[8];unsigned off=Header,losses=0;
    for(unsigned w=0;w<Wheels;++w)for(unsigned s=0;s<Sectors;++s){
        unsigned count=p[off++],selected=p[off++];
        if(count>Capacity||(count==0 && selected)||(count && selected>=count))return false;
        unsigned newCursor=0;
        for(unsigned i=0;i<Capacity;++i){
            Binding b{};b.formId=rw04::get32(p+off);off+=4;
            b.variantId=rw04::get32(p+off);off+=4;
            b.kind=p[off++];b.hand=p[off++];b.flags=p[off++];b.reserved=p[off++];
            if(i>=count){if(b.formId||b.variantId||b.kind||b.hand||b.flags||b.reserved)return false;continue;}
            if(!valid_binding(b))return false;
            unsigned mapped=0;
            if(!resolve(b.formId,&mapped,ctx)||!mapped){++losses;continue;}
            b.formId=mapped;
            if(!add(next.slot[w][s],b)){++losses;continue;}
            if(i<selected)++newCursor;
        }
        const unsigned size=next.slot[w][s].count;
        next.cursor[w][s]=size?static_cast<unsigned char>(newCursor<size?newCursor:size-1):0;
    }
    if(!validate(next))return false;
    target=next;
    if(dropped)*dropped=losses;
    return true;
}
inline bool decode_any(const unsigned char* p,unsigned n,State& state,Resolver resolve,void* ctx,unsigned* dropped=nullptr){
    if(n==PacketBytes)return decode(p,n,state,resolve,ctx,dropped);
    if(n==rw04::kBytes){
        rw04::State old{};
        if(!rw04::decode(p,n,old,resolve,ctx,dropped))return false;
        state=migrate(old);return validate(state);
    }
    return false;
}
}
