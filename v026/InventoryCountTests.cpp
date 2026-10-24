#include "InventoryCountPolicy.h"
#include <cassert>
int main(){
    int total=0;
    assert(rw121::merge_inventory_count(1,0,total) && total==1); // base-only shield
    assert(rw121::merge_inventory_count(2,-1,total) && total==1); // base and removal delta
    assert(rw121::merge_inventory_count(0,4,total) && total==4); // acquired in-game
    assert(!rw121::merge_inventory_count(1,-1,total)); // no longer owned
    assert(!rw121::merge_inventory_count(0,-10,total));
    assert(!rw121::merge_inventory_count(100000,100000,total));
    assert(!rw121::merge_inventory_count(-1,3,total));
}
