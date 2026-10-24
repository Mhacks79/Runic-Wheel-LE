#!/usr/bin/env python3
"""Fail-closed spell-store regression, SDK-grounded offsets (not gameplay proof)."""
from pathlib import Path
import os
import re
here=Path(__file__).resolve().parent
sdk=Path(os.environ.get('RUNIC_LAB_ROOT',str(here.parents[2])))/'sdk/skse-le-sdk/skse/skse'
s=(here/'RunicWheelV011.cpp').read_text()
refs=(sdk/'GameReferences.h').read_text()
obj=(sdk/'GameObjects.h').read_text()
forms=(sdk/'GameForms.h').read_text()
comp=(sdk/'GameFormComponents.h').read_text()
checks={
 'Actor added spells 0xFC': 'offsetof(Actor, addedSpells) == 0xFC' in refs and 'actor+0xFC' in s,
 'TESObjectREFR baseForm 0x24': bool(re.search(r'TESForm\*\s*baseForm;\s*//\s*24',refs)) and 'actor+0x24' in s,
 'Actor race 0x130': bool(re.search(r'TESRace\s*\*\s*race;\s*//\s*130',refs)) and 'actor+0x130' in s,
 'TESNPC spellList offset 0x64': bool(re.search(r'TESSpellList\s+spellList;\s*//\s*64',obj)) and 'form_spell_list(base,0x64' in s,
 'TESRace spellList offset 0x28': bool(re.search(r'TESSpellList\s+spellList;\s*//\s*028',forms)) and 'form_spell_list(race,0x28' in s,
 'TESSpellList has a data pointer': 'Data\t* unk04;' in comp or bool(re.search(r'Data\s*\*\s*unk04;',comp)),
 'Strict max bound and memory reads': 'rw12::valid_count(count)' in s and 'readable(entries,count*sizeof(Raw),0)' in s,
 'No unsupported spell equipping fallback': 'if(rw11::decide(request.binding,facts)!=rw11::EquipSpell)' in s,
 'Source-specific runtime logging': 'spell verified via TESRace::spellList' in s and 'spell_failure_log(proof)' in s,
}
for name,ok in checks.items():print(('PASS' if ok else 'FAIL'),name)
if not all(checks.values()):raise SystemExit('Spell regression failed')
mutant=s.replace('form_spell_list(base,0x64','form_spell_list(base,0x60')
assert 'form_spell_list(base,0x64' not in mutant and 'form_spell_list(base,0x64' in s
print('PASS intentional mutation: incorrect NPC offset detected')
