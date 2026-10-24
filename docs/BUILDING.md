# Building Runic Wheel LE from source

## Target (strict)

- Skyrim Legendary Edition / Classic: **TESV.exe 1.9.32.0.8**
- SKSE LE: **1.7.3**
- Windows **PE32**, Intel **x86**, MSVC-compatible 32-bit ABI
- C++17 and Direct3D 9 (D3D9 COM ABI)
- Optional **KHA-OS v0.5L** hosted configuration ABI

**Never** substitute Skyrim SE/AE offsets, SSE SKSE headers, 64-bit libraries or common SE hook addresses.

## Source code layout

- `src/RunicWheelV011.cpp`: native plugin entry points and engine integration
- `src/RunicKhaosMenu.inl`: optional KHA-OS settings UI
- `include/`: model, input, animation, rendering policy and serialization helpers
- `tests/`: static contracts, behavior tests and regressions
- `tools/`: build, import-stub and verification scripts
- `artwork/`: original texture-generation inputs and methods (as available)

## Prerequisites

You provide locally:
1. Licensed installation of TESV.exe 1.9.32.0.8 **outside this repository**.
2. SKSE LE 1.7.3 SDK headers and appropriate redistribution rights.
3. Clang/LLVM with `clang-cl`, `lld-link` and `llvm-objdump` or equivalent MSVC x86 toolchain.
4. Valid x86 Microsoft-compatible import libraries, Windows APIs and optional KHA-OS interface definitions.
5. Python 3 for QA scripts; additional packages such as `pefile` may be required for PE/signature checks.
6. Pillow and NumPy only if regenerating RWAT artwork.

The private SkyrimLE-DLL-RE-Laboratory workstation used during development is **not part of this public repository** and must not be uploaded to a public GitHub repository: it contains a private TESV.exe.

## Build procedure

The original development sources use `build-v025.sh`, which expects a lab root containing `sdk/skse-le-sdk`, `sdk/TESV.exe` (or the `TESV_EXE` environment variable), and toolchain/import-stub helpers.

With the appropriate dependencies prepared **locally**, run:

```bash
export RUNIC_LAB_ROOT="/path/to/your/private/laboratory"
export TESV_EXE="/path/to/private/TESV.exe"
export CLANG_CL="/path/to/clang-cl"
bash tools/build-v025.sh build/runicwheel
```

On Windows, use an appropriate shell environment or adapt the build for a native Visual Studio x86 target; Unix-style paths above are illustrative only. The scripts may require environment-specific adjustment. **Do not assume a fresh checkout builds independently without the stated SDK/toolchain.**

## Verification

The historical validation suite checks, as available:
- model input/scroll/stack state, serialization and KHA-OS window policy;
- x86 PE format, expected SKSE and optional KHA-OS exports;
- import and SEH properties;
- TESV.exe signature bytes for the exact LE executable hash;
- regression scenarios and source-integrity constraints.

Build success ≠ safe runtime hooks. A passing signature check ≠ proof of correct engine calling convention. **Only real in-game testing establishes gameplay behavior.**

Never commit `TESV.exe`, SKSE SDK files if their redistribution is not authorized, game-derived binary assets, private import libraries or build logs.
