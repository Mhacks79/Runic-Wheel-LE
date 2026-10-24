#!/usr/bin/env python3
"""Strict static tests. Build and binary checks do not prove in-game behavior."""
import os,sys,hashlib,subprocess,re
from pathlib import Path
import pefile
here=Path(__file__).resolve().parent
root=here.parents[2]
dll=Path(sys.argv[1]) if len(sys.argv)>1 else Path('/mnt/data/runic-v05-msvcabi/RunicWheelLE.dll')
obj=dll.with_name('RunicWheelV06.obj')
assert dll.is_file() and obj.is_file()
p=pefile.PE(str(dll))
assert p.FILE_HEADER.Machine==0x14c and p.FILE_HEADER.Characteristics&0x2000
assert p.OPTIONAL_HEADER.AddressOfEntryPoint==0
assert {x.name.decode() for x in p.DIRECTORY_ENTRY_EXPORT.symbols if x.name}=={'SKSEPlugin_Query','SKSEPlugin_Load'}
imp={x.dll.decode().lower():{i.name.decode() if i.name else str(i.ordinal) for i in x.imports} for x in p.DIRECTORY_ENTRY_IMPORT}
assert set(imp)=={'msvcrt.dll','kernel32.dll','user32.dll'},imp
assert '_except_handler3' in imp['msvcrt.dll']
assert 'GetPrivateProfileStringA' in imp['kernel32.dll']
assert 'AcquireSRWLockShared' in imp['kernel32.dll']
assert 'ReleaseSRWLockShared' in imp['kernel32.dll']
assert 'AcquireSRWLockExclusive' in imp['kernel32.dll']
assert 'ReleaseSRWLockExclusive' in imp['kernel32.dll']
assert not any(b'.CRT' in s.Name for s in p.sections)
llvm=Path(os.getenv('LLVM_MINGW_ROOT','/mnt/data/llvm-toolchains/llvm-mingw-20261020-msvcrt-ubuntu-22.04-x86_64'))
asm=subprocess.check_output([str(llvm/'bin/llvm-objdump'),'-d',str(obj)],text=True)
assert len(re.findall(r'%fs:0x0',asm))>=4
exe=root/'inputs/TESV.exe';assert exe.is_file()
assert hashlib.sha256(exe.read_bytes()).hexdigest()=='d43f3de477ee751caf36c26ee0a20a06225c29c08dfd13c827b537b9ac56cbf2'
image=pefile.PE(str(exe));ib=image.OPTIONAL_HEADER.ImageBase
def at(va,n):return image.get_data(va-ib,n)
assert at(0x00A68F86,5)==bytes.fromhex('E8 85 2C C8 FF')
assert at(0x00CE7D20,15)==bytes.fromhex('A1 F0 13 B9 01 8B 48 38 89 0D 28 09 BB 01 C3')
print('PASS v0.6: x86 MSVC-target PE, /NOENTRY, SEH, 2 exports, Win32 INI/SRWLOCK, TESV site signatures')
print('NOT TESTED: hardware, game runtime, FormID resolution, native equip')
