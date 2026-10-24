#pragma once
// Standalone bounded sum: TESContainer base + ExtraContainerChanges countDelta.
namespace rw121 {
inline bool merge_inventory_count(int base,int delta,int& total){
    if(base<0 || base>100000 || delta < -100000 || delta>100000) return false;
    total=base+delta;
    return total>=1 && total<=100000;
}
}
