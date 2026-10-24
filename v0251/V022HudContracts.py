#!/usr/bin/env python3
"""Strict contract checks for HUD context and late icon render ordering, plus ABI freeze."""
from pathlib import Path
p=Path(__file__).resolve().parent
old=(p.parent/'v021'/'RunicWheelV011.cpp').read_text()
cur=(p/'RunicWheelV011.cpp').read_text()
for start,end in [
    ('void perform_action(const ActionRequest& request){','int native_action_fault(){'),
    ('void save_impl(SKSESerializationInterface* serial){','void revert_impl(SKSESerializationInterface*)'),
    ('bool native_sites_verified(){','// The two safe pointers below'),
    ('void cycle_entry(int direction){','void snapshot_state_locked(rw05::State& state);'),
]:
    def segment(s): return s[s.index(start):s.index(end,s.index(start))]
    assert segment(old)==segment(cur), f'gameplay or co-save change: {start}'
assert 'g_artQuads[8*6]' in cur
assert 'g_miniArtQuads[8*6]' in cur
assert 'g_miniArtVert+6>8*6' in cur
assert 'ShowControlsGuide' in cur
assert 'ShowAllStackPreviews' in cur
assert 'ShowStackPositionDots' in cur
assert 'guideVisible=editContext?' in cur
assert 'rw22::guide_box(width,height,cx,cy,outer,editContext)' in cur
assert 'g_showAllStackPreviews || active' in cur
assert 'if(g_showNextPreview && slot.count>1 && (g_showAllStackPreviews || active))' in cur
assert 'add_mini_art_icon(bx,by,g_iconIndices[wheel][sector][next],outer,active);' in cur
assert 'g_miniArtVert=0' in cur
art=cur.index('if(ok && g_iconAtlas && g_artVertexCount)')
label=cur.index('if(ok && g_labelVert>0)')
mini=cur.index('if(ok && g_iconAtlas && g_miniArtVert)')
font=cur.index('if(ok && g_fontAtlas && g_fontVertexCount)')
assert art < label < mini < font, 'D3D9 layered compositing must be base art, backplates, mini art, font'
assert 'add_art_icon_scaled(px,py' not in cur, 'old mini must not reenter base pass'
ini=(p/'RunicWheelLE.ini').read_text()
for enabled in ['ShowNextItemPreview=1','ShowAllStackPreviews=1','ShowStackPositionDots=1',
                'ShowAdvancedEditorGuide=1','ShowControlsGuide=1']:
    assert enabled in ini, f'INI missing: {enabled}'
print('PASS v022 HUD: world+editor contextual guide; all stacked sectors; 8 bounded mini sprites; late D3D9 layer; exact native/save parity')
