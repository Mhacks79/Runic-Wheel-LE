#include "DisplayName.h"
#include <cassert>
#include <cstring>
#include <string>
int main(){
 char a[rw12ui::NameBytes]{};
 rw12ui::normalise_name("Hache d'arme",a);assert(std::strcmp(a,"HACHE D'ARME")==0);
 rw12ui::normalise_name(u8"Épée en ébène",a);assert(std::strcmp(a,u8"ÉPÉE EN ÉBÈNE")==0);
 rw12ui::normalise_name("\xc9p\xe9\xe5 en fer",a);assert(std::strcmp(a,u8"ÉPÉÅ EN FER")==0);
 rw12ui::normalise_name("  potion  de  soin ",a);assert(std::strcmp(a,"POTION DE SOIN")==0);
 rw12ui::normalise_name("",a);assert(!a[0]);
 rw12ui::normalise_name(nullptr,a);assert(!a[0]);
 std::string longInput(140,'A');rw12ui::normalise_name(longInput.c_str(),a);
 assert(std::strlen(a)<=rw12ui::NameBytes-1);
 rw12ui::normalise_name(u8"ÉÉÉÉÉÉÉÉÉÉÉÉÉÉÉÉÉÉÉÉÉÉÉÉ",a);
 assert(std::strlen(a)%2==0); // no split multi-byte glyph
}
