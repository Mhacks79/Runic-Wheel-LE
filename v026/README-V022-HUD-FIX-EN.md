# Runic Wheel LE 0.22.0 — HUD & Stack Visibility (private)

Target: Skyrim Legendary Edition 1.9.32.0.8 / SKSE 1.7.3 / Windows x86 / Direct3D 9.

## What changed

1. The right-side **controls guide now appears in world mode** (`ShowControlsGuide=1`).
   It uses a world-control list rather than exposing unavailable editor operations.
   In Inventory/Magic, the full 13-row Advanced Editor guide appears if
   `ShowAdvancedEditorGuide=1` and `EnableAdvancedEditorGestures=1`.
2. The guide is laid out on the right when possible, with a left-side fallback
   and a narrow-viewport in-frame fallback. The placement policy was tested at
   800x600, 1024x768, 1280x720, 1920x1080, 2560x1440, in both contexts.
3. The former next-item mini medallion was generated alongside main icons,
   then COVERED by opaque geometry in a later D3D9 pass. A new, bounded,
   separate 8-miniature pass is rendered after all backplate geometry and
   before Unicode glyphs, so previews cannot be obscured by their own ground.
4. All sectors with at least 2 stored items now show a next-item miniature.
   Option `ShowAllStackPreviews=0` restricts this to the active sector.
5. Tiny metallic stack-position markers are shown under next-item miniatures;
   toggle with `ShowStackPositionDots=0`.
6. New logs distinguish which guide context and whether the late miniature pass
   were really submitted by the D3D9 renderer, assisting follow-up QA.
7. All legacy editor/stack behavior, native TESV actions and SKSE serialization
   are source-verified identical to v0.21; no item consumption changes.

## New/important INI settings

- `ShowControlsGuide=1`: in-world help panel (new)
- `ShowAdvancedEditorGuide=1`: advanced editor panel while Inventory/Magic is open
- `ShowNextItemPreview=1`: master switch for next item miniatures
- `ShowAllStackPreviews=1`: miniature for all sectors with multiple entries (new)
- `ShowStackPositionDots=1`: stacked-entry progress pips (new)

## Status

The offline Win32 build, PE exports/imports, art and source contracts,
AddressSanitizer/UndefinedBehaviorSanitizer placement tests and 35,000 randomized
editor operations pass. Rendering and button behavior in actual Skyrim LE still
require runtime validation; the programmatic checks cannot emulate D3D9/ENB.

Skyrim/TESV.exe is deliberately not redistributed. This is not a stable/public release.
