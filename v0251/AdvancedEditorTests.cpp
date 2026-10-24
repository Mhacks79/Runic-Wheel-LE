#include "AdvancedEditor.h"
#include <cassert>
#include <cstdio>
using namespace rw05;
using namespace rw21;
static Binding item(unsigned id){Binding x{};x.formId=id;x.kind=Weapon;return x;}
int main(){
 Editor h{}; State s{}; Pending p{};
 assert(add(s.slot[0][0],item(1))&&add(s.slot[0][0],item(2)));
 assert(add(s.slot[0][1],item(3)));
 assert(count_occupied(s,0)==2 && count_entries(s,0)==3);
 assert(arm(p,s,MoveItem,0,0)==Armed);
 assert(paste(p,h,s,0,1)==Applied && p.mode==None);
 assert(s.slot[0][0].count==1 && s.slot[0][1].count==2);
 assert(s.slot[0][1].item[1].formId==1 && s.cursor[0][1]==1);
 assert(history(h,s,true) && s.slot[0][0].count==2 && s.slot[0][1].count==1);
 assert(history(h,s,false) && s.slot[0][0].count==1 && s.slot[0][1].count==2);
 assert(arm(p,s,CopyItem,0,1)==Armed);
 assert(paste(p,h,s,1,3)==Applied && s.slot[1][3].count==1);
 assert(arm(p,s,CopyItem,0,1)==Armed);
 assert(paste(p,h,s,1,3)==Duplicate && p.mode==CopyItem);
 cancel(p);
 assert(arm(p,s,SwapSector,0,1)==Armed);
 assert(paste(p,h,s,2,5)==Applied && s.slot[2][5].count==2 && !s.slot[0][1].count);
 assert(arm(p,s,CloneSector,2,5)==Armed);
 assert(paste(p,h,s,2,6)==Applied && s.slot[2][6].count==2);
 assert(arm(p,s,CloneSector,2,5)==Armed);
 assert(paste(p,h,s,2,6)==TargetFull);cancel(p);
 assert(rotate_entry(h,s,2,5,+1));
 assert(s.cursor[2][5]==0 || s.cursor[2][5]==1);
 assert(clear_stack(h,s,2,6) && !s.slot[2][6].count);
 assert(history(h,s,true) && s.slot[2][6].count==2);
 // Stale source must never mutate or copy another object.
 assert(arm(p,s,MoveItem,2,5)==Armed);
 s.slot[2][5].item[p.index]=item(999);
 const State prior=s;
 assert(paste(p,h,s,1,4)==Stale && p.mode==None);
 assert(s.slot[1][4].count==prior.slot[1][4].count);
 // Four entries maximum; never erase a destination or overwrite a full sector.
 s={};h={};p={};
 for(unsigned i=0;i<4;++i)assert(add(s.slot[0][0],item(i+1)));
 assert(add(s.slot[0][1],item(5)));
 assert(arm(p,s,MoveItem,0,1)==Armed);
 assert(paste(p,h,s,0,0)==TargetFull && s.slot[0][1].count==1);
 // Whole wheel management: clone only into a wholly empty wheel, swap without loss.
 s={};h={};p={};
 assert(add(s.slot[0][0],item(1)) && add(s.slot[0][1],item(2)));
 assert(!clone_wheel(h,s,0,0));
 assert(clone_wheel(h,s,0,1));
 assert(s.slot[1][0].item[0].formId==1 && s.slot[1][1].item[0].formId==2);
 assert(!clone_wheel(h,s,0,1));
 assert(swap_wheels(h,s,1,2));
 assert(!s.slot[1][0].count && s.slot[2][0].item[0].formId==1);
 assert(clear_wheel(h,s,2));
 assert(!s.slot[2][0].count && history(h,s,true) && s.slot[2][0].item[0].formId==1);
 // Opposite wheels do not share stack history/IDs.
 assert(validate(s));
 std::puts("PASS v021: atomic copy/move/swap/clone/clear/reorder/undo-redo, guards and counts");
}
