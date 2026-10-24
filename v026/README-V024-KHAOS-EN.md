# Runic Wheel LE 0.24 — KHA-OS Control Centre (Private Candidate)

A native x86 SKSE LE equipment wheel with an extensive **optional** KHA-OS v0.5L settings panel. This build is for **Skyrim Legendary Edition 1.9.32.0.8 + SKSE LE 1.7.3** (Direct3D 9, MSVC 32-bit ABI). KHA-OS is not redistributed; install the compatible host separately if you want the hosted menu. Without KHA-OS, the standalone wheel and its INI still work.

## What's new

**59 real, INI-backed controls** in eight categories:

1. **Layout (7):** 1–3 visible pages, 4–8 sectors per page, wheel diameter, X/Y screen position, inner selection deadzone, cursor response.
2. **Appearance (8):** Nordic heraldry art, forged decorative frame, large/small medallion sizes, wheel opacity, background strength, independent RGB highlight channels.
3. **Labels (6):** names, sector captions, text scaling, custom wheel titles, capacity display, stack progress hints.
4. **Stacks (6):** next-item medallion, miniature badges across populated sectors, progress dots, scroll direction and mouse codes.
5. **Animation (6):** master animations, reduced motion, opening/closing times, crossfade, feedback duration.
6. **Input (18):** middle mouse/side buttons, physical AZERTY/QWERTY scancodes, optional stack/occupied sector keys, hand preference, hold threshold, controller and deadzone, quick sector keys.
7. **Gameplay (6):** time slowdown, native equipment/spell gates, safe inventory UI capture, magic-school accents, diagnostics.
8. **Editor (2):** advanced editor control and inventory/magic-only controls guide.

Additional custom wheel names, six one-click palettes (**Nordic, Frost, Ember, Arcane, Forest, Silver**) and three **A/B/C** settings snapshots live in the same `RunicWheelLE.ini`. No extra ESP or settings file is created.

The preview in the hosted menu reflects the staged number of pages, sectors and highlight colour. Settings are staged until explicitly applied. Optional KHA-OS Obsidian theme background and scale are read as a copy; no ImGui object or context is shared between DLLs. Runic Wheel retains its own D3D9 rendering and SKSE lifecycle.

## Important real limits

**Save model is still RWL2, fixed at 3 pages × 8 sectors × 4 items per sector.** The 1–3 page and 4–8 sector options control how many of those existing positions are *visible and selectable*. Positions outside the visible limit remain present in saves and reappear when the limit is increased. They are **not** deleted. True page counts above 3, sector counts above 8 or more than 4 entries require a separate model, UI and co-save migration; they are not part of this candidate.

Not every setting can be applied before the next frame/device operation; the new live reconfiguration path is **not yet game validated**. Strongly recommended: use a test save and keep a backup of the old INI before applying new settings. Ingredients still have no verified LE native consumption route; they remain fail-closed.

## Integration

The module exposes SKSEPlugin_Query/Load and **KHAOS_Query, KHAOS_SetHosted, KHAOS_ToggleMenu, KHAOS_IsMenuOpen, KHAOS_RenderFrame, KHAOS_ProcessWndProc**. The ABI is taken from the user-supplied v0.5L `KhaosAPI.h`, API version 2. It has no static `KHAOS.dll` import, and its wheel works without KHA-OS.

1. Disable any previous Runic Wheel version in MO2 and install the provided ZIP with six files under `Data/SKSE/Plugins/`.
2. Leave **KHAOS.dll** managed by its own installation. Runic Wheel does not bundle or replace it.
3. Open the Runic Wheel module through the KHA-OS hub (actual in-game connection still needs a test).
4. Tab/PageUp/PageDown change categories; Up/Down choose a setting; Left/Right adjust; mouse click and wheel navigate. **Enter** applies, **Backspace** discards drafts, **End** resets the active tab to defaults, **Escape** closes.
5. **F6/F7/F8** save profiles A/B/C; **F9/F10/F11** stage those profiles (press Enter to apply). F2–F5 remain reserved. Wheel title editing uses the labels page and keyboard input.

## Building using the generic laboratory

Extract the `v024/` source folder under `<SkyrimLE-DLL-RE-Laboratory>/work/RunicWheelLE/v024/`, or point the `RUNIC_LAB_ROOT` environment variable at the lab root. Run `bash build-v024.sh work/_v024_build` in a compatible environment with `clang-cl`, `lld-link`, `llvm-ar`, `llvm-objdump`, Python, the SKSE LE SDK and your private `TESV.exe` in `sdk/TESV.exe`. The script runs the extensive prior regression suites, the new geometry/API tests, creates minimal Win32 import libraries and builds PE32 with /EHa.

**Validation status:** built, statically qualified and tested offline. **Not tested in-game with KHA-OS**. No evidence is claimed for real Hub appearance, device reset or live frame ordering until tested in Skyrim.
