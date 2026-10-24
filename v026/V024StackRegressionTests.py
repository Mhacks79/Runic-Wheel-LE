#!/usr/bin/env python3
"""Safety and integration guard for v0.20. Run before Windows build."""
from pathlib import Path
p=Path(__file__).resolve().parent
s=(p/'RunicWheelV011.cpp').read_text()
ini=(p/'RunicWheelLE.ini').read_text()
assert 'rw20::accept(g_scrollEdges[wheelIndex],b->value,b->duration,GetTickCount())' in s
assert 'if(down || b->duration==0.0f)' not in s
assert 'if(changed){\n        visual_feedback(9)' in s
assert 'g_model.slot[wheel][sector].item[after].formId' in s
assert 'STACK W0 S0 0/0 -> 0/0 FORM=00000000' in s
assert 'select_next_occupied(int direction)' in s
assert 'rw20::next_filled(counts,sector,direction)' in s
assert 'rw20::preview_index(index,slot.count)' in s
assert 'g_showNextPreview && slot.count>1 && (g_showAllStackPreviews || active)' in s
assert 'add_mini_art_icon(bx,by,g_iconIndices[wheel][sector][next],outer,active)' in s
assert 'ShowNextItemPreview=1' in ini
assert 'ShowMagicSchoolAccents=1' in ini
assert 'PreviousOccupiedSectorScanCode=0' in ini
assert 'NextOccupiedSectorScanCode=0' in ini
assert 'g_miniArtQuads[8*6]' in s and 'g_miniArtVert+6>8*6' in s
assert 'const DWord tint=id==rw14::Alteration?' in s
assert 'game thread' not in (p/'ScrollPulsePolicy.h').read_text()
print('PASS V020: no release double step, real FormID diagnostics, eight-sector late-pass bounds, preview, next filled sector and school accent integration')
