#pragma once
// Form-aware VISUAL selection; never used for gameplay equipment decisions.
// Values verified in Skyrim LE SKSE headers GameObjects.h / GameFormComponents.h.
namespace rw14 {
enum Art : unsigned char {
 Sword=0,Dagger,Axe,Mace,Greatsword,Warhammer,Bow,Crossbow,
 Staff,Shield,Armor,Helmet,Gauntlet,Boots,Ring,Amulet,
 Alteration,Conjuration,Destruction,Illusion,Restoration,Power,
 Potion,Poison,Food,Ingredient,Shout,Arrows,Bolts,Torch,Misc,Scroll,
 Count
};
inline unsigned char icon_for_spell(unsigned spellType,unsigned spellSchool){
 // SpellItem::type (LE 1.9.32): 0 spell, 2 power, 3 lesser power, 4 ability,
 // 6 addition, 7 voice. EffectSetting::school stores actor-value ids;
 // Skyrim magic skills are commonly AV 18..22, but some toolchains surface 0..4.
 switch(spellType){
  case 2: case 3: case 4: case 6: return Power;
  case 7: return Shout;
 }
 switch(spellSchool){
  case 18: case 0: return Alteration;
  case 19: case 1: return Conjuration;
  case 20: case 2: return Destruction;
  case 21: case 3: return Illusion;
  case 22: case 4: return Restoration;
  default: return Power; // generic fallback remains visibly magical
 }
}
inline unsigned char icon_for(unsigned formType,unsigned weaponType,unsigned armorMask,
                              unsigned alchemyFlags,unsigned ammoFlags,
                              unsigned spellType=0xFFFFFFFFu,unsigned spellSchool=0xFFFFFFFFu){
 switch(formType){
 case 0x29: switch(weaponType){
    case 2:case 12:return Dagger;
    case 3:case 13:return Axe;
    case 4:case 14:return Mace;
    case 5:case 15:return Greatsword;
    case 6:case 16:return Warhammer;
    case 7:case 17:return Bow;
    case 8:case 18:return Staff;
    case 9:case 19:return Crossbow;
    default:return Sword;
   }
 case 0x1A:
   if(armorMask&(1u<<9))return Shield;
   if(armorMask&((1u<<0)|(1u<<12)))return Helmet;
   if(armorMask&(1u<<3))return Gauntlet;
   if(armorMask&(1u<<7))return Boots;
   if(armorMask&(1u<<6))return Ring;
   if(armorMask&(1u<<5))return Amulet;
   return Armor;
 case 0x2E:
   if(alchemyFlags&0x2u)return Food;
   if(alchemyFlags&0x00020000u)return Poison;
   return Potion;
 case 0x1E:return Ingredient;
 case 0x1F:return Torch; // LE LIGH, validated inventory ownership required
 case 0x2A:return (ammoFlags&0x4u)?Arrows:Bolts; // SDK kNotBolt=1<<2
 case 0x16:return icon_for_spell(spellType,spellSchool);
 case 0x77:return Shout;
 case 0x1B:return Scroll; // BOOK/scroll generic -- non-equippable category fallback
 default:return Misc;
 }
}
inline const char* spell_caption(unsigned art){
 switch(art){
  case Alteration:return "ALTERATION";
  case Conjuration:return "CONJURATION";
  case Destruction:return "DESTRUCTION";
  case Illusion:return "ILLUSION";
  case Restoration:return "RESTORATION";
  case Power:return "POWER";
  case Shout:return "DRAGON SHOUT";
  default:return "SPELL";
 }
}

}
