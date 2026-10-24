#!/usr/bin/env python3
"""Version-specific checks against the exact player-validated v0.23 implementation."""
from pathlib import Path
p=Path(__file__).resolve().parent
before=(p/'REFERENCE/v023-RunicWheelV011.cpp').read_text()
after=(p/'RunicWheelV011.cpp').read_text()
for start,end in [
 ('void perform_action(const ActionRequest& request){','int native_action_fault(){'),
 ('void save_impl(SKSESerializationInterface* serial){','void revert_impl(SKSESerializationInterface*)'),
 ('bool native_sites_verified(){','// The two safe pointers below')]:
 def segment(s):return s[s.index(start):s.index(end,s.index(start))]
 assert segment(before)==segment(after),f'Unreviewed native gameplay/save delta: {start}'
assert 'guideVisible=editContext && g_showEditorHelp && g_enableAdvancedEditor' in after
assert 'guide_box(width,height,cx,cy,outer,true)' in after
assert 'ShowControlsGuide' not in after
assert 'g_miniArtQuads[8*6]' in after
assert 'g_firstThemeEnabled && g_iconAtlas && g_artVertexCount' in after
art=after.index('if(ok && g_firstThemeEnabled && g_iconAtlas && g_artVertexCount)')
label=after.index('if(ok && g_labelVert>0)')
mini=after.index('if(ok && g_firstThemeEnabled && g_iconAtlas && g_miniArtVert)')
font=after.index('if(ok && g_fontAtlas && g_fontVertexCount)')
assert art<label<mini<font
assert 'kSaveVersion=2' in after
print('PASS V024: native action and RWL2 co-save byte-for-byte source preserved, mini icon rendering order and editor-only guide')
