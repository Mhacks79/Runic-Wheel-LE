# Runic Wheel LE 0.15 — Forged Nordic interface (private visual test)

**Target:** Skyrim Legendary Edition 1.9.32.0.8 / SKSE 1.7.3 (Windows x86).
**Known good gameplay rollback:** Runic Wheel LE 0.12.1, verified by the player for ordinary equipment, shield, spells, jump/animations, slowdown and saving.

## What's actually new

This is a genuine visual-layer iteration of the first approved dark-Nordic concept, not a repackaged mockup or a Wheeler port. The executable uses **32 original independently drawn subtype icons** (sword, dagger, axe, mace, greatsword, warhammer, bow, crossbow, staff, shield, separate armor slots, elemental magic, potion, food, ingredient, shout, and more). Each icon is generated separately with consistent lighting, engraved steel-and-gold rims and transparent gutters; no sheet cropping. The renderer draws a custom engraved translucent radial frame, a separate metal-bordered nameplate and a Unicode serif font atlas. Its original central medallion is isolated with a tested alpha mask from the **first user-approved AI-generated concept image**; no landscape or screenshot wedges are used. The central selected-item name is prioritized: tiny per-sector names are **off by default** (`ShowSectorLabels=0`) and can be enabled if preferred. Standard `ShowItemNames=0` hides names and also deactivates the optional name-read task. The wheel layout accounts for the bottom plaque and keeps the elements on-screen at 720p–1440p, even when WheelScalePercent=170.

### Required MO2 files

Install exactly one `RunicWheelLE.dll` and its matching INI and four texture files together:

* `RunicWheelIcons.rwtx` (2048×1024 BGRA), 32 unique icons
* `RunicWheelFrame.rwtx` (1024×1024 BGRA), transparent metalwork
* `RunicWheelFont.rwtx` (1024×1024 BGRA), independently rasterized serif glyphs including French accents
* `RunicWheelPlaque.rwtx` (1024×256 BGRA), new nameplate

All texture files use `RWAT` + little-endian width/height + uncompressed BGRA data, validated before packaging. Their backgrounds are genuinely transparent; no screenshot of a game scene is overlaid.

## Safety and control invariants

* The `perform_action` source block remains **identical** to 0.12.1, including its game-thread native calls, spell-specific `EquipSpell`, item ownership checks and fail-closed behavior.
* Existing bindings, menu-context editing, wheel stacks, SKSE co-save, `SlowTimePercent`, and F2–F5 passthrough are unchanged.
* Texture load failure leaves an older geometric icon/sector fallback. Missing plaque falls back to a simple bar.
* `ShowConceptIcons=0` disables the theme; the key is retained for older INIs. `ShowSectorLabels=1` optionally enables small names inside sectors.
* Do **not** install this over a working installation with a second DLL. Use a separate MO2 profile and disposable save. Revert to 0.12.1 for any regression.

## Verification and limitations

Static tests check 32 distinct icon bodies, alpha gutters, subtype classification (including armor shield and food vs. potion vs. ingredient), UTF-8 normalization, font glyph coverage, texture headers and exact byte sizes, four layout previews (720p, 1080p on dark/light simulated backgrounds, 1440p), and 16 viewport/scale combinations. Wrong-type and wrong-default mutations must fail. `CommonLibLE verify.py` still reports **525 rows, every address holds**. Native C++ Release and sanitizer suites pass. The Win32 PE32 DLL compiles with MSVC ABI, protected exceptions (SEH) and both SKSE exports. Source comparison proves unchanged equipment requests.

**These are offline tests.** Screenshots in `art/V015-UI-*` are manually simulated layouts, not screenshots from a running game. ENB interaction, real D3D9 texture appearance and the visual effect of selection still require a single in-game visual review. This is **not** a public final release, nor a pixel-perfect reproduction of the original approved painting. The gameplay baseline 0.12.1 remains the rollback reference.

## Artwork and attribution

The 32 vector glyphs and wheel ornament geometry are independently authored for Runic Wheel LE; the center is adapted from the first user-approved AI-generated visual reference. No Wheeler or Bethesda artwork/code is included. The only font-derived content distributed is bitmap glyph pixels rasterized offline from DejaVu Serif; no font binaries are packaged.
