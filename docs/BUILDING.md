# Native build — Runic Wheel LE v0.25.1

## Exact target

- Skyrim **Legendary Edition 1.9.32.0.8**, SKSE **LE 1.7.3**.
- Windows PE32 **x86**; MSVC-compatible ABI and SEH.
- Native C++17, Direct3D 9 COM device interfaces.
- Optional KHA-OS v0.5L host via separately obtained interface headers.

## Current source tree

The development ZIP stores the real code under `v0251/`, **not** in a fabricated split `src/` or `include/` hierarchy.

- `v0251/RunicWheelV011.cpp`: native plugin implementation, SKSE lifecycle, D3D9 rendering.
- `v0251/RunicKhaosMenu.inl`: hosted KHA-OS control centre.
- `v0251/*.h`: model, serialization, input policies, rendering and guarded native action helpers.
- `v0251/*Tests.cpp`, `v0251/*Tests.py`: offline tests and contract checks.
- `v0251/art/`, `v0251/artwork/`: original image sources and RWAT generation.
- `v0251/quality/`, `v0251/qa/`: validation tools.
- `v0251/build-v025.sh`, `v0251/verify-v024.py`, `v0251/generate_v024_imports.py`: final build path, PE checks and import-stub generation.

**The original v0.25.1 source files are published.** Building still requires the external dependencies, proprietary game executable and authorized KHA-OS SDK described below.

## Prerequisites — provided privately

1. Legally obtained `TESV.exe` (exact build 1.9.32.0.8), **never put it on public GitHub**.
2. Official SKSE **LE 1.7.3** SDK, used with redistribution restrictions respected.
3. `clang-cl`, `lld-link`, `llvm-objdump` or an equivalent correctly configured MSVC x86 toolchain.
4. Valid Win32/MSVC-compatible import libraries as required for linking.
5. Original KHA-OS interface headers **`KhaosAPI.h` and `KhaosThemeAPI.h`** obtained from an authorized KHA-OS SDK.
6. Python, `pefile`, plus Pillow/NumPy if regenerating images.

The private `SkyrimLE-DLL-RE-Laboratory` is not a public source dependency or redistributable repository component.

## Build from the private lab layout

The existing script derives the lab root by navigating three parents above `v0251`. Place the source directory under the private lab as `work/RunicWheelLE/v0251/`, and ensure `RUNIC_LAB_ROOT` points to that lab:

```bash
export RUNIC_LAB_ROOT="/absolute/path/to/your/private/SkyrimLE-DLL-RE-Laboratory"
export TESV_EXE="/absolute/path/to/private/TESV.exe"
export CLANG_CL="/absolute/path/to/clang-cl"
bash v0251/build-v025.sh work/_runic_build
```

On native Windows, use a compatible shell or port these commands to your existing MSVC x86 build configuration. Paths and toolchain discovery may need adaptation. The original script calls many offline tests and the x86 linker; the `verify-v024.py` naming is historical.

## Acceptance checks

- Native PE32/x86 DLL, no unexpected entrypoint;
- both `SKSEPlugin_Query` / `SKSEPlugin_Load` exports, plus optional six hosted KHA-OS exports;
- expected Win32 imports and SEH layout;
- signatures match the **exact** TESV executable SHA-256;
- serialization/inventory/stack actions, input, UI layout and KHA-OS focused-window regressions;
- asset integrity, shader-independent artwork and correct alpha;
- source/build manifest plus SHA-256.

**Compilation ≠ runtime validation**. Static address evidence ≠ validated hook behavior. Test the precise generated binary in a dedicated LE profile with save backups.
