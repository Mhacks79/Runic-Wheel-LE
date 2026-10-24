#!/usr/bin/env python3
"""Offline enforcement: no world panel; enlarged, legible editor UI only, never touch native actions."""
from pathlib import Path
p=Path(__file__).resolve().parent
before=(p.parent/'v022'/'RunicWheelV011.cpp').read_text()
after=(p/'RunicWheelV011.cpp').read_text()
for start,end in [
 ('void perform_action(const ActionRequest& request){','int native_action_fault(){'),
 ('void save_impl(SKSESerializationInterface* serial){','void revert_impl(SKSESerializationInterface*)'),
 ('bool native_sites_verified(){','// The two safe pointers below'),
 ('void cycle_entry(int direction){','void snapshot_state_locked(rw05::State& state);')]:
 def segment(s):return s[s.index(start):s.index(end,s.index(start))]
 assert segment(before)==segment(after),f'Native or co-save drift: {start}'
assert 'guideVisible=editContext && g_showEditorHelp && g_enableAdvancedEditor' in after
assert 'guide_box(width,height,cx,cy,outer,true)' in after
assert 'ShowControlsGuide' not in after
assert 'WORLD controls guide RENDERED' not in after
assert 'professional_text(keyCenter' in after and 'professional_text(actionCenter' in after
assert '17.1f*k,key' in after and '18.5f*k,actions' in after
assert 'g_miniArtQuads[8*6]' in after
art=after.index('if(ok && g_iconAtlas && g_artVertexCount)')
label=after.index('if(ok && g_labelVert>0)')
mini=after.index('if(ok && g_iconAtlas && g_miniArtVert)')
font=after.index('if(ok && g_fontAtlas && g_fontVertexCount)')
assert art < label < mini < font
ini=(p/'RunicWheelLE.ini').read_text()
assert 'ShowAdvancedEditorGuide=1' in ini
assert '\nShowControlsGuide=' not in ini
print('PASS v023: inventory/magic-only, two-column text, legacy world guide disabled, native/co-save frozen, mini-art late layer')
