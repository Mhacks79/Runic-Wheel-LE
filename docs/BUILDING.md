# Native build - Runic Wheel LE v0.26.0

## Exact target

- Skyrim **Legendary Edition 1.9.32.0.8**, SKSE **LE 1.7.3**.
- Windows PE32 **x86**, Microsoft C++ compiler, static runtime, real SEH (`/EHa`).
- Native C++17, Direct3D 9 COM device interfaces.
- Optional KHA-OS v0.5L host through separately obtained interface headers.

## Source tree

Everything lives in `v026/`:

- `RunicWheelV011.cpp`: plugin, SKSE lifecycle, engine actions, D3D9 rendering.
- `RunicKhaosMenu.inl`: hosted KHA-OS control centre.
- `*.h`: model, co-save, input, item-copy, rendering and action policies.
- `*Tests.cpp`, `*Tests.py`, `*Contracts.py`: offline tests and contract checks.
- `art/`, `artwork/`: original image sources and RWAT generation.
- `REFERENCE/`: earlier versions the contract scripts compare against.
- `build-v026.bat`: the build.

## Prerequisites

1. Visual Studio 2022 Build Tools with the x86 C++ tools.
2. The official SKSE **LE 1.7.3** sources (only their headers are used).
3. `KhaosAPI.h` and `KhaosThemeAPI.h` from the KHA-OS integration kit, copied into `v026/`.
4. Python 3.11 for the contract scripts. A legally owned `TESV.exe` 1.9.32.0.8 for the ones that read the game executable (`TESV_EXE`).

## Build

```bat
set SKSE_LE_SDK=<folder of the SKSE LE 1.7.3 sources>
v026\build-v026.bat <output folder>
```

`VCVARS32` can point to another `vcvars32.bat` if Build Tools are not in the default place.

## Acceptance checks

- PE32/x86 DLL, `SKSEPlugin_Query` / `SKSEPlugin_Load` plus the six KHA-OS exports, imports limited to kernel32 and user32;
- every engine site the plugin calls matches the exact TESV executable (`V026Contracts.py`);
- co-save code byte-for-byte the v0.25.1 one;
- host tests and contract scripts pass, and each negative control fails as it should;
- asset integrity and correct alpha.

**Compilation is not runtime validation.** Test the exact DLL in a dedicated LE profile with save backups.
