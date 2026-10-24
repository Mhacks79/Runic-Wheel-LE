#!/usr/bin/env python3
"""Static PE/signature gate for v0.10 test candidate; NOT a gameplay claim."""
from pathlib import Path
import hashlib, os, re, subprocess, sys, struct
import pefile
here=Path(__file__).resolve().parent
root=here.parents[2]
dll=Path(sys.argv[1]); obj=dll.with_suffix('.obj')
assert dll.exists() and obj.exists(), 'DLL or matching object missing'
p=pefile.PE(str(dll))
assert p.FILE_HEADER.Machine==0x14c, 'not x86'
assert p.FILE_HEADER.Characteristics & 0x2000, 'not DLL'
assert p.OPTIONAL_HEADER.AddressOfEntryPoint==0, 'unexpected entry point'
exports={x.name.decode() for x in p.DIRECTORY_ENTRY_EXPORT.symbols if x.name}
assert exports=={'SKSEPlugin_Query','SKSEPlugin_Load'}, exports
imports={x.dll.decode().lower():{(y.name.decode() if y.name else str(y.ordinal)) for y in x.imports}
         for x in p.DIRECTORY_ENTRY_IMPORT}
assert set(imports)=={'msvcrt.dll','kernel32.dll','user32.dll'}, imports
assert '_except_handler3' in imports['msvcrt.dll'], 'SEH x86 absent'
assert 'AcquireSRWLockShared' in imports['kernel32.dll']
assert 'GetTickCount' in imports['kernel32.dll']
assert not any(b'.CRT' in section.Name for section in p.sections), 'unexpected CRT init section'
llvm=Path(os.getenv('LLVM_MINGW_ROOT','/mnt/data/toolchain/llvm-mingw-20261020-msvcrt-ubuntu-22.04-x86_64'))
asm=subprocess.check_output([str(llvm/'bin/llvm-objdump'),'-d',str(obj)],text=True)
assert len(re.findall(r'%fs:0x0',asm))>=4, 'real x86 SEH prologues not present'
exe=root/'inputs/TESV.exe'
assert hashlib.sha256(exe.read_bytes()).hexdigest()=='d43f3de477ee751caf36c26ee0a20a06225c29c08dfd13c827b537b9ac56cbf2'
engine=pefile.PE(str(exe));base=engine.OPTIONAL_HEADER.ImageBase
at=lambda a,n:engine.get_data(a-base,n)
assert at(0x00A68F86,5)==bytes.fromhex('E8 85 2C C8 FF'), 'input hook site differs'
assert at(0x00CE7D20,15)==bytes.fromhex('A1 F0 13 B9 01 8B 48 38 89 0D 28 09 BB 01 C3'),'DX9 site differs'
assert at(0x00A4B150,5)==bytes.fromhex('80 79 2C 00 74'),'BSTimer site differs'
assert at(0x006E3E30,8)==bytes.fromhex('53 8B 5C 24 08 57 8B F9'),'menu event site differs'
source=(here/'RunicWheelV010.cpp').read_text()
assert 'hook_menu_render' not in source and 'install_ui_read_hooks' not in source
assert 'SKSETaskInterface* g_tasks' in source
assert 'AddUITask((UIDelegate_v1*)&g_uiTask)' in source
assert 'AddTask((TaskDelegate*)&g_timeTask)' in source
assert 'g_timeActive' in source and 'InterlockedCompareExchange(&g_timeBusy,1,0)' in source
assert 'if(key && b->scan>=0x3C && b->scan<=0x3F)return 0;' in source
print('PASS v0.10: PE32/x86, MSVC ABI, real SEH, two SKSE exports, signed LE sites, UI task without Render detours')
print('NOT TESTED in Skyrim: UI selected item path; slowdown+restore; save/load; gamepad; no native equipment')
