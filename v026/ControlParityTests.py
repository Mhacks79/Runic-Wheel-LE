#!/usr/bin/env python3
"""Static parity vs dTry Wheeler src/bin/Config.h and SKSE LE skse/InputMap.cpp.
Does NOT claim controller gameplay support or complete editing UX.
No Wheeler source is bundled: only factual command identifiers are checked.
"""
from pathlib import Path
import re
import sys
HERE=Path(__file__).parent
cpp=(HERE/'RunicWheelV011.cpp').read_text()
ini=(HERE/'RunicWheelLE.ini').read_text()

# From original Wheeler Config.h: virtual key IDs 280 LT, 281 RT, 275 RB,
# 274 LB, 268 DPAD-L, 269 DPAD-R. SKSE LE InputMap.cpp maps them to
# actual ButtonEvent.keyMask: LT=9 RT=10 RB=0x200 LB=0x100 DPAD-L=4 DPAD-R=8.
expected={
    'OpenMouseButton':2,            # USER-requested change vs Wheeler original CapsLock
    'OpenKeyboardScanCode':0,       # Unbound: avoids toggling Skyrim CapsLock walk/run
    'GamepadOpenMask':9,            # LT 280
    'GamepadPreviousMask':0,        # Unbound in Wheeler original
    'GamepadNextMask':10,           # RT 281
    'GamepadPrimaryMask':512,       # RB 275
    'GamepadSecondaryMask':256,     # LB 274
    'GamepadPreviousItemMask':4,    # DPAD left 268
    'GamepadNextItemMask':8,        # DPAD right 269
    'GamepadRightStickMask':12,     # ThumbstickEvent right
    'KeyboardLayoutMode':2,         # User improvement for AZERTY/QWERTY, not original
    'PreviousWheelScanCode':16,    # Q on US / A on FR AZERTY (physical key position)
    'NextWheelScanCode':18,        # E on US
    'PrimaryMouseButton':0,        # LMB 256 original
    'SecondaryMouseButton':1,      # RMB 257 original
    'CycleEntryUpMouseCode':8,     # Wheel up 264 original
    'CycleEntryDownMouseCode':9,   # Wheel down 265 original
}

def read_config(config):
    result={}
    for match in re.finditer(r'^\s*([A-Za-z][\w]*)\s*=\s*(\d+)\s*(?:;[^\n]*)?$',config,re.M):
        result[match.group(1)]=int(match.group(2))
    return result

def parity_issues(config,source):
    options=read_config(config);errors=[]
    for key,expected_value in expected.items():
        if options.get(key)!=expected_value:
            errors.append(f'{key} expected {expected_value}, got {options.get(key)}')
    # Prevent mismatch where INI documents inputs runtime does not actually consume.
    for key in ('GamepadPreviousItemMask','GamepadNextItemMask'):
        if f'value("{key}"' not in source:errors.append(f'{key} not read by native plugin')
    if 'cycle_entry(b->scan==(DWord)g_cfg.padItemPrevious?-1:1)' not in source:
        errors.append('DPAD item navigation not dispatched')
    if '(pad && g_cfg.padPrev && b->scan==(DWord)g_cfg.padPrev)' not in source:
        errors.append('unbound pad wheel previous not safely disabled')
    if 'g_cfg.keyboardOpen>=0x3C && g_cfg.keyboardOpen<=0x3F' not in source or 'if(key && b->scan>=0x3C && b->scan<=0x3F)return 0;' not in source:
        errors.append('F2-F5 passthrough missing')
    if re.search(r'^\s*Editor(?:Toggle|Undo|Redo|Tool)ScanCode\s*=',config,re.M):
        errors.append('legacy F-key editing config present')
    return errors

def run():
    assert not parity_issues(ini,cpp), '\n'.join(parity_issues(ini,cpp))
    # Negative checks guarantee a regression is detected, including old V0.9 button map.
    for name,wrong in [('GamepadPrimaryMask',4096),('GamepadNextMask',512),('GamepadOpenMask',16384),('GamepadPreviousItemMask',0)]:
        mutated=re.sub(rf'(?m)^({name}=)\d+',rf'\g<1>{wrong}',ini)
        assert parity_issues(mutated,cpp),f'{name} mutation not detected'
    assert parity_issues(ini,cpp.replace('cycle_entry(b->scan==(DWord)g_cfg.padItemPrevious?-1:1)','')), 'missing DPAD dispatcher not detected'
    print('PASS: 18 keyboard/mouse/gamepad original-map defaults, runtime DPAD hookup, F2-F5 guard, 5 negative controls')
    print('LIMIT: not a controller-in-game test; N/M and reordering actions not implemented; UI-thread hovered-item read awaits in-game validation')

if __name__=='__main__':
    try:run()
    except AssertionError as exc:print('FAIL:',exc,file=sys.stderr);sys.exit(1)
