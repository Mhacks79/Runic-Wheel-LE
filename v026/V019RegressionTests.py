#!/usr/bin/env python3
"""Static/API-invariant regression tests, not a substitute for Skyrim runtime tests."""
from pathlib import Path
import re,struct,hashlib
p=Path(__file__).resolve().parent
s=(p/'RunicWheelV011.cpp').read_text()
ini=(p/'RunicWheelLE.ini').read_text()
st=(p/'StackNavigation.h').read_text()
spell=(p/'IconPolicy.h').read_text()
checks={
 'canonical cursor updated': 'changed=rw19::cycle(g_model,wheel,sector,direction);' in s,
 'legacy projection updated': 'InterlockedExchange(&g_cursors[wheel][sector],(Long)after)' in s,
 'selected item pointer inherited unchanged': 'binding=rw05::current(snapshot,snapshot.activeWheel' in s,
 'stack-position persisted in RWL2': 'state.cursor[w][sec]=(unsigned char)rw02::clamp' in s,
 'thread-safe write': 'AcquireSRWLockExclusive(&g_catalogLock);\n    __try {\n        count=g_model.slot[wheel][sector].count;' in s and 'changed=rw19::cycle(g_model,wheel,sector,direction);' in s,
 'custom wheel titles': 'g_wheelTitles[wheel]' in s and 'ShowWheelTitles' in ini,
 'stack hints': 'g_showStackHints && active && slot.count>1' in s,
 'keyboard item hotkeys': 'PreviousItemScanCode=0' in ini and 'NextItemScanCode=0' in ini,
 'reverse scroll': 'ReverseItemScroll=0' in ini and 'g_reverseItemScroll?-1:1' in s,
 'F2-F5 remains reserved': 'if(key && b->scan>=0x3C && b->scan<=0x3F)return 0;' in s,
 'magic-school captions': 'rw14::spell_caption(iconId)' in s and 'case Restoration:return "RESTORATION"' in spell,
 'effect scan bounded': 'effectCount>32' in s and 'readable(entries,effectCount*sizeof(Raw),0)' in s,
 'deferred native engine actions unchanged': 'g_tasks->AddTask((TaskDelegate*)&g_nativeTask);' in s,
 'ingredient routed like the vanilla inventory (v0.26)': 'ingredient request submitted (verify effect in game)' in s,
 'illustration base bundled': (p/'art/V017-Heraldry-Base.png').is_file(),
}
for label,good in checks.items():print(('PASS' if good else 'FAIL')+': '+label)
assert all(checks.values())
art=(p/'RunicWheelIcons.rwtx').read_bytes()
assert art[:4]==b'RWAT' and struct.unpack_from('<II',art,4)==(2048,1024)
assert len(art)==12+2048*1024*4
print('PASS: 2048x1024 RWAT artwork,',hashlib.sha256(art).hexdigest())
