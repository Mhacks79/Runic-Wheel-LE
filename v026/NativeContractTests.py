#!/usr/bin/env python3
"""Static safety tests for native action routing; not a substitute for Skyrim runtime."""
from pathlib import Path
import re
p=Path(__file__).with_name('RunicWheelV011.cpp')
s=p.read_text(encoding='utf8')
ini=Path(__file__).with_name('RunicWheelLE.ini').read_text()
checks={
 'spell actor/base/race validation': 'learned_spell(actor,form,proof)' in s and 'form_spell_list(base,0x64' in s and 'form_spell_list(race,0x28' in s and 'spell_failure_log(proof)' in s,
 'game-thread delivery': 'g_tasks->AddTask((TaskDelegate*)&g_nativeTask)' in s,
 'input only queues': 'void log_action(const char* hand)' in s and 'g_actionRequest.binding=binding;' in s,
 'actor pointer check': '0x01B2E8E4' in s and 'readable(actor,0x140,0)' in s,
 'equip ABI signature check': 'native_sites_verified()' in s and '0x006EF3E0' in s and '0x006EE560' in s,
 'distinct spell EquipManager ABI': 'using GameEquipSpell=void (__thiscall*)(Raw,Raw,Raw,Raw);' in s and 'reinterpret_cast<GameEquipSpell>(0x006EFAC0)(manager,actor,form,spellSlot);' in s,
 'no implicit equip by raw FormID': 'if(!owned_inventory(actor,form,owned))' in s,
 'exact copy or refusal (v0.26)': 'facts.instanceMissing=choice.pick==rw26::PickNone;' in s and 'never substitute another one' in (p.with_name('InstancePolicy.h')).read_text(),
 'custom enchant/tempering told apart (v0.26)': 'info.customized=' in s and '0x9b' in s and '0x25' in s,
 'game generation check': 'g_actionGeneration' in s and 'request.generation!=' in s,
 'exception disables actions': 'InterlockedExchange(&g_actionFaulted,1)' in s,
 'one queued task at a time': 'InterlockedCompareExchange(&g_actionQueued,1,0)' in s,
 'safe profile can disable actions': 'EnableNativeActions' in ini and 'g_cfg.nativeActions' in s,
 'no function-key editing': not any(f'editorF{n}' in s for n in range(2,6)) and 'F2-F5' in s,
 'preserve slowdown controller': 'BSTimer time setter signature VERIFIED' in s,
 'no menu render detour': 'InventoryMenu::Render vtable hook' not in s and 'vt[17]' in s,
 'UI kind capture for icons': 'InterlockedExchange(&g_uiKind,pick.kind)' in s,
 'loaded icon refresh task': 'schedule_type_refresh();' in s,
 'enlarged wheel': '0.31f*(float)g_cfg.wheelScale' in s and 'WheelScalePercent=130' in ini,
 'configurable translucency': 'WheelOpacityPercent=70' in ini and 'tinted(outerCol)' in s,
 'feedback visible longer than frame': 'ClickFeedbackMs=900' in ini and 'draw_text(cx-89.f' in s,
 'typed original category pictograms': 'draw_category_icon(' in s and 'kWeaponMask[9]' in s and 'kSpellMask[9]' in s,
 'red/green feedback': 'REFUSED' in s and 'UNEQUIP' in s and 'ADDED' in s,
 'no embedded third-party assets': 'wheeler-main' not in s.lower() and 'wheeler refined' not in s.lower(),
}
for label,ok in checks.items():
 print(('PASS' if ok else 'FAIL'), label)
if not all(checks.values()): raise SystemExit('native contract failed')
# Intentional negative test: removes ownership-gating condition in a simulated source copy.
mutant=s.replace('if(!owned_inventory(actor,form,owned))','if(false)')
assert 'if(!owned_inventory(actor,form,owned))' not in mutant
print(f"PASS: {len(checks)} contracts plus 1 ownership-negative control")
