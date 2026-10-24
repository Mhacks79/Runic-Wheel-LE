from pathlib import Path
import re
s = (Path(__file__).parent/'RunicWheelV011.cpp').read_text()
i = (Path(__file__).parent/'RunicWheelLE.ini').read_text()
assert 'if(!g_enableSlowTime || !g_menuSinkReady)return;' not in s
assert 'if(!g_enableSlowTime && !g_timeActive)return;' in s
assert 'rw09::wants_slowdown(g_visible!=0' in s
assert 'g_cfg.keyboardOpen>=0x3C && g_cfg.keyboardOpen<=0x3F' in s
assert 'if(key && b->scan>=0x3C && b->scan<=0x3F)return 0;' in s
assert 'EnableSafeInventoryUITask=1' in i
assert 'g_hoverCaptureEnabled=value("EnableSafeInventoryUITask",1,0,1);' in s
assert 'install_ui_read_hooks' not in s
assert 'hook_menu_render' not in s
assert 'g_tasks->AddUITask' in s
assert 'queue_ui_sample' in s
for legacy in ('EditorToggleScanCode', 'EditorUndoScanCode', 'EditorRedoScanCode', 'EditorToolScanCode'):
    assert not re.search(r'^\s*'+legacy+r'\s*=',i,re.M)
    assert not re.search(r'\bvalue\s*\(\s*"'+legacy+r'"',s)
assert 'OpenMouseButton=2' in i and 'SlowTimePercent=10' in i
print('PASS: independent slowdown, 4 function keys reserved, old bindings absent, no Render detour; UI task enabled with bounded queue')
