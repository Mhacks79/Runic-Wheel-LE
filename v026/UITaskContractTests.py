from pathlib import Path
import re
p=Path(__file__).parent
source=(p/'RunicWheelV011.cpp').read_text()
ini=(p/'RunicWheelLE.ini').read_text()

def errors(s,config):
    issues=[]
    if 'hook_menu_render(' in s or 'install_ui_read_hooks(' in s:issues.append('unsafe Render detour remains')
    if 'AddUITask((UIDelegate_v1*)&g_uiTask)' not in s:issues.append('no SKSE UI task')
    if 'AddTask((TaskDelegate*)&g_timeTask)' not in s:issues.append('no main-thread time restore')
    if 'InterlockedCompareExchange(&g_uiTaskQueued,1,0)' not in s:issues.append('duplicate task possible')
    if 'InterlockedExchange(&g_uiTaskQueued,0)' not in s:issues.append('task slot never released')
    if 'g_tasks->interfaceVersion<2' not in s:issues.append('SKSE task version unchecked')
    if 'if(!rw10::valid_capacity(capacity)' not in s:issues.append('unbounded menu-table scan')
    if 'run_ui_task' not in s or '__except(ui_read_fault())' not in s:issues.append('UI task fault unguarded')
    if 'clear_ui_value(v)' not in s:issues.append('managed scaleform data leaked')
    if 'GetVariable>(vt[17])' not in s:issues.append('GFx ABI slot unverified')
    if 'EnableSafeInventoryUITask=1' not in config:issues.append('default UI task option missing')
    if any(x in s for x in ('EditorToggleScanCode','EditorToolScanCode','EditorUndoScanCode','EditorRedoScanCode')):issues.append('old editor keys still active')
    return issues
assert errors(source,ini)==[],errors(source,ini)
for marker in ['InterlockedCompareExchange(&g_uiTaskQueued,1,0)','__except(ui_read_fault())','g_tasks->AddUITask((UIDelegate_v1*)&g_uiTask)','if(!rw10::valid_capacity(capacity)']:
    changed=source.replace(marker,'/* removed by mutation */')
    assert errors(changed,ini),marker
print('PASS UI task contract: no Render hooks, bounded menu scan, backpressure, version check, SEH + four negative mutations')
