#pragma once
// Independent and bounded spell-membership policy. Pointer readability is checked
// by the game-thread adapter BEFORE this function is ever called.
namespace rw12 {
enum Source : unsigned {None=0,ActorAdded=1,NpcBase=2,Race=3};
static const unsigned kMaximumSpells=4096;
inline bool valid_count(unsigned count){return count>0 && count<=kMaximumSpells;}
template<typename Pointer>
inline bool contains(Pointer const* entries,unsigned count,Pointer needle){
    if(!entries || !needle || !valid_count(count))return false;
    for(unsigned i=0;i<count;++i)if(entries[i]==needle)return true;
    return false;
}
}
