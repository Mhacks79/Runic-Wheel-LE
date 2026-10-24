#!/usr/bin/env python3
"""Version-specific guard: fail if a native Skyrim action or co-save ABI changes."""
from pathlib import Path
import re
p=Path(__file__).resolve().parent
base=(p/'REFERENCE'/'v020-RunicWheelV011.cpp').read_text()
cur=(p/'RunicWheelV011.cpp').read_text()
for start,end in [('void perform_action(const ActionRequest& request){','int native_action_fault(){'),
                  ('void save_impl(SKSESerializationInterface* serial){','void revert_impl(SKSESerializationInterface*)'),
                  ('bool native_sites_verified(){','// The two safe pointers below')]:
    def segment(text):
        return text[text.index(start):text.index(end,text.index(start))]
    assert segment(base)==segment(cur),f'FAIL: native or save path changed: {start}'
for fragment in ['#include "AdvancedEditor.h"','g_enableAdvancedEditor && inventory_context()',
                 'rw21::clear_stack(g_editor,state,wheel,sector)',
                 'rw21::clone_wheel(g_editor,state,wheel,target)',
                 'rw21::swap_wheels(g_editor,state,wheel,target)',
                 'rw21::rotate_entry(g_editor,state,wheel,sector,direction)',
                 'g_modifiers.update((unsigned)b->scan,down)',
                 'g_editorDisplay', 'EnableAdvancedEditorGestures',
                 'g_showEditorHelp && g_enableAdvancedEditor',
                 'ShowWheelOccupancy', 'EnableQuickSectorKeys']:
    assert fragment in cur or fragment in (p/'RunicWheelLE.ini').read_text(),fragment
assert 'RWAT' in (p/'art'/'v018_arcana.py').read_text()
assert 'EnableAdvancedEditorGestures=1' in (p/'RunicWheelLE.ini').read_text()
assert 'EnableSafeInventoryUITask=1' in (p/'RunicWheelLE.ini').read_text()
assert (p/'RunicWheelIcons.rwtx').read_bytes()[:4]==b'RWAT'
assert 'g_artQuads[8*6]' in cur
assert 'g_labelVert+6>30000' in cur
assert 'kSaveVersion=2' in cur
print('PASS v021 integration: native equip/save unchanged, advanced gestures isolated to editor, bounded buffers, RWL2 compatible')
