#!/usr/bin/env python3
"""v0.11.2 fail-closed spell route gate; exact Skyrim LE binary, not runtime gameplay proof."""
from pathlib import Path
import hashlib
import os
import re
import pefile
here=Path(__file__).resolve().parent
root=here.parents[2]
source=(here/'RunicWheelV011.cpp').read_text()
exe=Path(os.environ.get('TESV_EXE',str(here.parents[2]/'inputs/TESV.exe')))
assert hashlib.sha256(exe.read_bytes()).hexdigest()=='d43f3de477ee751caf36c26ee0a20a06225c29c08dfd13c827b537b9ac56cbf2','wrong TESV.exe'
pe=pefile.PE(str(exe))
rd=lambda a,n: pe.get_data(a-pe.OPTIONAL_HEADER.ImageBase,n)
# Native MagicMenu distinguishes SpellItem (type 0x16) and Shout (type 0x77).
assert rd(0x00873A80,6)==bytes.fromhex('8A 45 12 3C 16 75'), 'MagicMenu type check drift'
assert rd(0x00873A91,14)==bytes.fromhex('50 55 51 8B 0D AC 5F 2E 01 E8 21 C0 E7 FF'), 'MagicMenu spell equip call site drift'
assert rd(0x006EFAC0,12)==bytes.fromhex('83 EC 08 53 8B 5C 24 18 55 56 8B 74'), 'spell method prologue drift'
assert rd(0x006EFB46,3)==b'\xc2\x0c\x00' and rd(0x006EFB5A,3)==b'\xc2\x0c\x00', 'spell method must clean 12 stack bytes'
assert 'using GameEquipSpell=void (__thiscall*)(Raw,Raw,Raw,Raw);' in source,'wrong spell ABI declaration'
assert 'reinterpret_cast<GameEquipSpell>(0x006EFAC0)(manager,actor,form,spellSlot);' in source,'native SpellItem must route to EquipSpell'
assert 'rw26::spell_takes_hand(spellType)?slot:nullptr' in source,'v0.26: powers get no slot, like the MagicMenu'
assert 'if(!g_enableSpellActions)' in source and 'EnableSpellActions' in (here/'RunicWheelLE.ini').read_text(),'missing spell-specific rollback switch'
assert 'sizeof(equipSpell)' in source and '((unsigned char*)0x006EFAC0)[i]!=equipSpell[i]' in source, 'missing runtime signature gate'
start=source.index('if(kind==rw05::Spell){',source.index('void perform_action('))
end=source.index('if(kind==rw05::Shout){',start)
spell_section=source[start:end]
assert 'learned_spell(actor,form,proof)' in spell_section,'learned spell gate missing'
assert 'GameEquipSpell' in spell_section and '0x006EFAC0' in spell_section
assert 'reinterpret_cast<GameEquip>' not in spell_section, 'old generic EquipItem on SpellItem is unsafe'
assert 'result needs game validation' in spell_section,'must not claim gameplay success'
# Regression mutant recreates the exact old failure: generic native equip for a spell.
mutant=source.replace('reinterpret_cast<GameEquipSpell>(0x006EFAC0)(manager,actor,form,spellSlot);',
   'reinterpret_cast<GameEquip>(0x006EF3E0)(manager,actor,form,nullptr,1,slot,true,false,false,nullptr);')
mstart=mutant.index('if(kind==rw05::Spell){',mutant.index('void perform_action(')); mend=mutant.index('if(kind==rw05::Shout){',mstart)
assert 'reinterpret_cast<GameEquip>' in mutant[mstart:mend] and 'reinterpret_cast<GameEquip>' not in spell_section, 'mutation gate ineffective'
print('PASS: exact LE MagicMenu spell dispatch, ret0C ABI, runtime signature, fail-closed spell route')
print('PASS: mutation recreating v0.11.1 wrong EquipItem is rejected')
print('GAME RUNTIME: not tested by these static checks')
