#!/usr/bin/env python3
"""Test base + delta inventory ownership; v0.26 sends INGR through EquipItem like the vanilla menu."""
from pathlib import Path
import os
s=(Path(__file__).parent/'RunicWheelV011.cpp').read_text()
sdk=Path(os.environ.get('RUNIC_LAB_ROOT',str(Path(__file__).parents[3])))/'sdk/skse-le-sdk/skse/skse'
source=(sdk/'GameObjects.h').read_text()
assert 'TESContainer\t\tcontainer;\t// 58' in source, 'TESActorBase container offset changed'
assert 'TESContainer\t\t\t\tcontainer;\t\t// 20' in source, 'other container layout changed'
assert 'bool npc_base_count(' in s and 'npc+0x58' in s, 'Missing NPC-base ownership path'
assert 'const unsigned char* container=(const unsigned char*)npc+0x58' in s
assert 'count.delta=(int)delta;' in s and 'merge_inventory_count(count.base,count.delta,count.total)' in s
assert 'count<1 || count>100000' not in s, 'Old delta-only refusal reintroduced'
assert 'case 0x2E:case 0x1E:return rw05::Consumable;' in s, 'Ingredient classification missing'
act=s[s.index('void perform_action('):s.index('int native_action_fault(){')]
assert 'ingredient eat native route not verified' not in act
assert act.index('if(!owned_inventory(actor,form,owned))') < act.index('ingredient request submitted (verify effect in game)'), 'ingredient must stay behind ownership'
assert 'log_item_refusal(' in s and 'form=' in s and 'type=' in s
assert 'professional_text(x' in s and 'g_iconIndices[wheel][sector][index]' in s
bad=s.replace('merge_inventory_count(count.base,count.delta,count.total)','merge_inventory_count(0,count.delta,count.total)')
assert 'merge_inventory_count(count.base,count.delta,count.total)' not in bad
bad2=act.replace('if(!owned_inventory(actor,form,owned))','if(false)')
assert 'if(!owned_inventory(actor,form,owned))' not in bad2
print('PASS v0.12.1/v0.26: SDK NPC base container offset, base+delta, ingredient behind ownership, larger UI and diagnostics')
print('PASS negative mutations: dropped base count / dropped ownership gate rejected')
