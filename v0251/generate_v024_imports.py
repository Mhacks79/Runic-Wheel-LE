#!/usr/bin/env python3
"""Minimal reproducible MSVC/x86 Windows import libraries for Runic Wheel LE v0.15.

Generates *real* COFF short-import objects whose linker symbols use stdcall/cdecl
x86 decoration, but whose PE imported function names are undecorated. This is a
self-contained replacement for the three MinGW import archives, NOT a substitute
for a Windows in-game validation.
"""
import argparse
from pathlib import Path
import shutil
import struct
import subprocess

# v0.15's exact original PE imports and calling conventions.
FUNCS = {
    'msvcrt': {
        'memcpy': '_memcpy', 'memset': '_memset', 'memmove': '_memmove',
        '_except_handler3': '__except_handler3'
    },
    'kernel32': {
        'InterlockedCompareExchange': '_InterlockedCompareExchange@12',
        'GetTempPathA': '_GetTempPathA@8',
        'CreateFileA': '_CreateFileA@28',
        'CloseHandle': '_CloseHandle@4',
        'GetModuleHandleExA': '_GetModuleHandleExA@12',
        'GetModuleFileNameA': '_GetModuleFileNameA@12',
        'OutputDebugStringA': '_OutputDebugStringA@4',
        'GetPrivateProfileIntA': '_GetPrivateProfileIntA@16',
        'GetPrivateProfileStringA': '_GetPrivateProfileStringA@24',
        'InterlockedExchange': '_InterlockedExchange@8',
        'WriteFile': '_WriteFile@20',
        'AcquireSRWLockShared': '_AcquireSRWLockShared@4',
        'ReleaseSRWLockShared': '_ReleaseSRWLockShared@4',
        'AcquireSRWLockExclusive': '_AcquireSRWLockExclusive@4',
        'ReleaseSRWLockExclusive': '_ReleaseSRWLockExclusive@4',
        'VirtualProtect': '_VirtualProtect@16',
        'GetCurrentProcess': '_GetCurrentProcess@0',
        'FlushInstructionCache': '_FlushInstructionCache@12',
        'GetCurrentThreadId': '_GetCurrentThreadId@0',
        'GetTickCount': '_GetTickCount@0',
        'VirtualQuery': '_VirtualQuery@12',
        'GetProcAddress':'_GetProcAddress@8',
        'WritePrivateProfileStringA':'_WritePrivateProfileStringA@16',
        'ReadFile': '_ReadFile@20'
    },
    'user32': {
        'IsWindow': '_IsWindow@4',
        'GetForegroundWindow': '_GetForegroundWindow@0',
        'GetKeyboardLayout': '_GetKeyboardLayout@4',
        'VkKeyScanExA': '_VkKeyScanExA@8',
        'MapVirtualKeyExA': '_MapVirtualKeyExA@12'
    }
}

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('dest',type=Path,help='mini-toolchain root')
    a=parser.parse_args()
    tools={x:shutil.which(x) for x in ('lld-link','llvm-objdump','llvm-ar')}
    if not all(tools.values()):
        raise SystemExit(f'Missing local compiler utilities: {tools}')
    libdir=a.dest/'i686-w64-mingw32'/'lib'
    binpath=a.dest/'bin'
    objs=a.dest/'_import_objects'
    for folder in (libdir,binpath,objs):folder.mkdir(parents=True,exist_ok=True)
    for cmd in ('lld-link','llvm-objdump'):
        link=binpath/cmd
        if link.is_symlink() or link.exists():link.unlink()
        link.symlink_to(tools[cmd])
    for module, exports in FUNCS.items():
        files=[]
        for real_name, coff_name in exports.items():
            # Import object: IMAGE_FILE_MACHINE_I386, CODE type, NAME_UNDECORATE.
            # On link: '__imp__Foo@N' resolves to PE import 'Foo' from real DLL.
            data=(coff_name+'\0'+module+'.dll'+'\0').encode('ascii')
            header=struct.pack('<HHHHIIHH',0,0xffff,0,0x14c,0,len(data),0,3<<2)
            path=objs/(module+'_'+real_name+'.obj')
            path.write_bytes(header+data)
            files.append(str(path))
        lib=libdir/('lib'+module+'.a')
        subprocess.run([tools['llvm-ar'],'rcs','--format=coff',str(lib),*files],check=True)
        print(f'{module}.dll: {len(exports)} imports, {lib.stat().st_size} bytes -> {lib}')
    print('Minimal v0.15 link dependencies restored (PE32/x86, MSVC ABI).')

if __name__=='__main__':main()
