#include "IconPolicy.h"
#include <cassert>
int main(){
 using namespace rw14;
 const unsigned expected[]={Sword,Sword,Dagger,Axe,Mace,Greatsword,Warhammer,Bow,Staff,Crossbow};
 for(unsigned t=1;t<=9;t++)assert(icon_for(0x29,t,0,0,0)==expected[t]);
 assert(icon_for(0x1A,0,1u<<9,0,0)==Shield);
 assert(icon_for(0x1A,0,1u<<2,0,0)==Armor);
 assert(icon_for(0x1A,0,1u<<0,0,0)==Helmet);
 assert(icon_for(0x1A,0,1u<<3,0,0)==Gauntlet);
 assert(icon_for(0x1A,0,1u<<7,0,0)==Boots);
 assert(icon_for(0x1A,0,1u<<6,0,0)==Ring);
 assert(icon_for(0x1A,0,1u<<5,0,0)==Amulet);
 assert(icon_for(0x2E,0,0,2,0)==Food);
 assert(icon_for(0x2E,0,0,0x20000,0)==Poison);
 assert(icon_for(0x2E,0,0,0,0)==Potion);
 assert(icon_for(0x1E,0,0,0,0)==Ingredient);
 assert(icon_for(0x2A,0,0,0,4)==Arrows);
 assert(icon_for(0x2A,0,0,0,0)==Bolts);
 assert(icon_for(0x16,0,0,0,0,0,18)==Alteration);
 assert(icon_for(0x16,0,0,0,0,0,19)==Conjuration);
 assert(icon_for(0x16,0,0,0,0,0,20)==Destruction);
 assert(icon_for(0x16,0,0,0,0,0,21)==Illusion);
 assert(icon_for(0x16,0,0,0,0,0,22)==Restoration);
 assert(icon_for(0x16,0,0,0,0,2,0xFFFFFFFFu)==Power);
 assert(icon_for(0x16,0,0,0,0,7,0xFFFFFFFFu)==Shout);
 assert(icon_for(0x77,0,0,0,0)==Shout);
 assert(icon_for(0x1B,0,0,0,0)==Scroll);
 assert(icon_for(0xDE,0,0,0,0)==Misc);
}
