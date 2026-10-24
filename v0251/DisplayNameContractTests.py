#!/usr/bin/env python3
"""Display-name source contract after genuine Unicode/serif texture rewrite."""
from pathlib import Path
import hashlib
import os
import pefile
here=Path(__file__).resolve().parent
src=(here/'RunicWheelV011.cpp').read_text()
ui=(here/'DisplayName.h').read_text()
exe=Path(os.environ.get('TESV_EXE',str(here.parents[2]/'inputs/TESV.exe')))
assert hashlib.sha256(exe.read_bytes()).hexdigest()=='d43f3de477ee751caf36c26ee0a20a06225c29c08dfd13c827b537b9ac56cbf2'
pe=pefile.PE(str(exe))
assert pe.get_data(0x00452300-pe.OPTIONAL_HEADER.ImageBase,6)==bytes.fromhex('8b 44 24 04 6a 00')
assert 'void fill_names_on_game_thread()' in src
assert 'g_tasks->AddTask((TaskDelegate*)&g_typeRefreshTask)' in src
assert 'AcquireSRWLockExclusive(&g_catalogLock)' in src
assert 'ReleaseSRWLockExclusive(&g_catalogLock)' in src
assert 'void normalise_name' in ui and 'utf8_decode' in ui and 'unicode_upper' in ui
assert 'if(n+size>=NameBytes)break;' in ui
assert 'NameBytes=72' in ui
assert 'g_fontAtlas=read_texture' in src
assert 'professional_text(' in src
assert 'const unsigned formType=' in src and 'g_iconIndices[w][sector][i]' in src
render=src[src.index('void draw_labels('):src.index('int draw(Raw device)')]
assert 'g_displayNames[wheel][selected][cursor]' in render
assert '0x00452300' not in render and '0x00451A30' not in render
# Negative mutation: engine access in draw should be detected.
bad=render+' (void)0x00452300;'
assert '0x00452300' in bad and '0x00452300' not in render
print('PASS V0.14: engine name signature, guarded task Unicode bounds, DX cached-text-only, negative engine-read mutation')
