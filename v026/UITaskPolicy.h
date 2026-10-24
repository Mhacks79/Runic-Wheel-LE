#pragma once
// Pure model of UI thread safety gates; no engine calls or allocation.
namespace rw10 {
inline bool valid_capacity(unsigned n) {return n>=1u && n<=2048u && (n&(n-1u))==0u;}
inline bool sample_allowed(unsigned menuFlags,bool enabled,unsigned elapsedMs){
    return enabled && (menuFlags&3u)!=0u && elapsedMs>=90u;
}
}
