# Runic Wheel LE v0.15 — Original visual rebuild (private test)

**Target:** Skyrim Legendary Edition 1.9.32.0.8, SKSE 1.7.3 (x86).
**Reference:** Player-confirmed v0.12.1. This release changes visual rendering and reads form subtype data exclusively inside the existing guarded SKSE game task. It does not change `perform_action`/gameplay, controls, saved record schema or native spell call.

## Visual direction

The approved first concept: smoky, semi-transparent sectors, restrained dark Nordic metal, antique gold engravings, 32 independently authored illustrations and a bright readable serif caption. The mock-up concept is a **reference**, not a texture to crop. Actual game sprites are rebuilt individually from original vectors, aligned on a 256px tile with 18px transparent gutters.

* Weapon subtype: sword, dagger, axe, mace, greatsword, warhammer, bow, crossbow, staff.
* Armor slots: shield, body armor, helmet/circlet, gauntlets, boots, ring, amulet.
* ALCH flags: food, poison, potion. `INGR` gets its own flower icon, not the potion icon.
* Spell/scroll/shout, arrows/bolts and miscellaneous are mapped separately.
* Item names support uppercase French accented characters including É, È, À, Ç, Ù and ligatures. Legacy Windows-1252 and UTF-8 strings are handled without splitting Unicode bytes.
* Text automatically truncates to width with a Unicode ellipsis; the full selected-item caption is placed below the wheel.

## DX9 files (same folder as DLL)

* `RunicWheelIcons.rwtx`: 2048×1024, 32 independent BGRA roundels (8×4).
* `RunicWheelFrame.rwtx`: 1024×1024 BGRA ornament/rim/center.
* `RunicWheelFont.rwtx`: 1024×1024 BGRA bitmap font, no TTF/OTF distribution.

All RWAT assets have a 12-byte header: ASCII `RWAT`, width uint32-LE, height uint32-LE, followed by raw BGRA pixel rows. D3DPOOL_MANAGED textures are loaded once per valid device and drawn with restored texture states. Any absent layer falls back to the previous geometry where possible. Images in `art/UI-PRO-PREVIEW-*.png` are **offline layout simulations**, not proof of actual Skyrim rendering.

## How to use

Install with MO2 as an isolated mod, replacing any previous Runic Wheel DLL so only one version is active. Preserve a separate throwaway game save. There is **no change** to inventory editing (left/right click), mouse wheel, gamepad, quicksave F5 passthrough, time slowdown or native spell actions.

`ShowConceptIcons=0` disables the professional RWAT visual layer (setting name intentionally retained for compatibility). `ShowItemNames=0` disables labels. If a visual failure occurs, revert to player-confirmed v0.12.1 rather than changing the native action settings.

## Validation limitations

PE32/x86 ABI and SEH, native action code byte-for-byte source comparison, host Release + ASan/UBSan, icon taxonomy tests, per-sprite isolation, Unicode glyph coverage and 3 preview sizes were checked offline. **The v0.15 renderer has not been observed on the user's Skyrim system.** A compile/test pass cannot establish ENB compatibility, GPU reset behavior or practical legibility on every display. Keep the public-release gate closed pending in-game evidence.

All art/symbols are original for Runic Wheel LE; no assets/code extracted from Wheeler or Bethesda. Serif glyph textures are rendered offline from DejaVu Serif, which is not distributed as a font file. Source artwork and generation scripts are supplied separately for reproducibility.
