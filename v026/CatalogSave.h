#pragma once
// Runic Wheel LE: original, allocation-free, versioned, deterministic co-save payload.
// Format is explicit LE bytes; NEVER reinterpret native structs as disk records.
#include "SlotCatalog.h"
namespace rw04 {
constexpr unsigned kSlots=rw03::Wheels*rw03::Slots;
constexpr unsigned kSlotBytes=2+4*rw03::Entries;
constexpr unsigned kHeader=16;
constexpr unsigned kBytes=kHeader+kSlots*kSlotBytes;
static_assert(kBytes==448,"format changes require a new co-save version");
struct State {
    rw03::Catalog catalog{};
    unsigned char cursor[rw03::Wheels][rw03::Slots]{};
    unsigned char activeWheel=0;
};
inline unsigned crc32(const unsigned char* p,unsigned count,unsigned seed=0xFFFFFFFFu){
    unsigned crc=seed;
    for(unsigned i=0;i<count;++i){
        crc^=p[i];
        for(unsigned j=0;j<8;++j)crc=(crc>>1)^((0u-(crc&1u))&0xEDB88320u);
    }
    return crc;
}
inline void put32(unsigned char* p,unsigned x){
    for(int i=0;i<4;++i)p[i]=(unsigned char)(x>>(8*i));
}
inline unsigned get32(const unsigned char* p){
    return (unsigned)p[0]|((unsigned)p[1]<<8)|((unsigned)p[2]<<16)|((unsigned)p[3]<<24);
}
// Magic RWL1, version1, dimensions and active wheel; checksum includes entire header except CRC field.
inline void encode(const State& state,unsigned char (&out)[kBytes]){
    for(unsigned i=0;i<kBytes;++i)out[i]=0;
    out[0]='R';out[1]='W';out[2]='L';out[3]='1';out[4]=1;
    out[5]=rw03::Wheels;out[6]=rw03::Slots;out[7]=rw03::Entries;
    out[8]=state.activeWheel<rw03::Wheels?state.activeWheel:0;
    unsigned pos=kHeader;
    for(int w=0;w<rw03::Wheels;++w)for(int s=0;s<rw03::Slots;++s){
        const rw03::Slot& slot=state.catalog.slots[w][s];
        const unsigned n=slot.count<=rw03::Entries?slot.count:0;
        out[pos++]=(unsigned char)n;
        out[pos++]=n?(unsigned char)(state.cursor[w][s]%n):0;
        for(unsigned i=0;i<rw03::Entries;++i){put32(out+pos,i<n?slot.form[i]:0);pos+=4;}
    }
    unsigned crc=crc32(out,12);
    crc=~crc32(out+kHeader,kBytes-kHeader,crc);
    put32(out+12,crc);
}
// Form IDs are resolved through SKSE on load, rather than reused from a stale load order.
// On missing forms compact entries; on corrupt packets reject the entire record transactionally.
// Resolver signature: bool resolve(unsigned saved, unsigned* current, void* context).
using Resolver=bool (*)(unsigned,unsigned*,void*);
inline bool decode(const unsigned char* bytes,unsigned size,State& output,Resolver resolver,void* context,unsigned* dropped=nullptr){
    if(!bytes||size!=kBytes||!resolver)return false;
    if(bytes[0]!='R'||bytes[1]!='W'||bytes[2]!='L'||bytes[3]!='1'||bytes[4]!=1||
       bytes[5]!=rw03::Wheels||bytes[6]!=rw03::Slots||bytes[7]!=rw03::Entries||bytes[8]>=rw03::Wheels||
       bytes[9]||bytes[10]||bytes[11])return false;
    unsigned crc=crc32(bytes,12);
    crc=~crc32(bytes+kHeader,kBytes-kHeader,crc);
    if(crc!=get32(bytes+12))return false;
    State loaded{};loaded.activeWheel=bytes[8];
    unsigned pos=kHeader,skipped=0;
    for(int w=0;w<rw03::Wheels;++w)for(int s=0;s<rw03::Slots;++s){
        unsigned n=bytes[pos++],cursor=bytes[pos++];
        if(n>rw03::Entries|| (n==0&&cursor!=0)|| (n>0&&cursor>=n))return false;
        rw03::Slot& slot=loaded.catalog.slots[w][s];
        unsigned retainedBeforeCursor=0;
        for(unsigned i=0;i<rw03::Entries;++i){
            unsigned id=get32(bytes+pos);pos+=4;
            if(i>=n){if(id)return false;continue;}
            if(!id)return false;
            unsigned resolved=0;
            if(!resolver(id,&resolved,context)||!resolved||!rw03::add(slot,resolved)){
                ++skipped;
            } else if(i<cursor) {
                ++retainedBeforeCursor;
            }
        }
        loaded.cursor[w][s]=slot.count?(unsigned char)(retainedBeforeCursor<slot.count?retainedBeforeCursor:slot.count-1):0;
    }
    output=loaded;
    if(dropped)*dropped=skipped;
    return true;
}
}
