#pragma once
// Pure context-name classifier; no engine ownership and no GPU/thread dependencies.
namespace rw08 {
inline bool equals(const char*a,const char*b){if(!a||!b)return false;while(*a&&*b){if(*a++!=*b++)return false;}return *a==0&&*b==0;}
inline unsigned identify(const char* s){
  return equals(s,"InventoryMenu")?1u:(equals(s,"MagicMenu")?2u:0u);
}
inline unsigned transition(unsigned previous,unsigned flag,bool opening){
  return opening?(previous|flag):(previous&~flag);
}
inline bool editor(unsigned flags){return (flags&3u)!=0;}
inline bool slowdown(bool wheelOpen,unsigned flags){return wheelOpen && !editor(flags);}
}
