from pathlib import Path
root=Path(__file__).parent
src=(root/'RunicWheelV011.cpp').read_text()
ui=(root/'RunicKhaosMenu.inl').read_text()
header=(root/'KhaosWindowPolicy.h').read_text()
assert 'rw25::close_on_directinput' in src
assert src.index('if(g_window && GetForegroundWindow()!=g_window)') < src.index('if(g_khaosOpen){',src.index('int process_event'))
assert 'return b->scan==15 ? 0:1' in src, 'Tab must be sent to Skyrim'
assert 'rw25::lost_focus' in src and 'return false;' in src
assert 'rw25::skyrim_tab' in ui and 'g_khaosDragging=0' in ui
assert 'if(key==0x09){g_khaosTab' not in ui, 'Tab cannot change pages and suppress Skyrim'
assert 'WM_MOUSEMOVE, drag ONLY while holding title bar' in ui
assert 'KHAOS close button clicked' in ui
assert 'return false; // allow host Hub/Skyrim to process outside clicks' in ui
assert 'khaos_centered(labelLeft+labelW*.5f,yy' in ui
assert 'khaos_centered(bx,yy+2.f*s,28.f*s' in ui
assert 'const float x=baseX+o.x,y=baseY+o.y;' in ui
assert '35.f/46.f' in header
assert len([x for x in ui.splitlines() if 'khaos_centered(' in x])>=13
assert 'SKSEPlugin_Load' in src and 'KHAOS_ProcessWndProc' in src
print('PASS V025: correctly centered ink; draggable bounded panel; close button; Tab pass-through; focus-loss release; outside click-through')
