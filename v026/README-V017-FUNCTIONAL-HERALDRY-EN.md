# Runic Wheel LE 0.17.0 — Forged Nordic Functional + Heraldry (PRIVATE)

Target: Skyrim Legendary Edition 1.9.32.0.8, SKSE 1.7.3, 32-bit D3D9. Install with MO2 as a replacement for v0.16 (do NOT enable both). All user-data bindings remain in the per-character SKSE co-save (RWL2, version 2).

## Changes after player feedback

1. **Hand click reversed IN THE WORLD.** Left click = right hand; right click = left hand. `LeftClickEquipsRightHand=1` is configurable. Inventory/Magic edit mode is deliberately unchanged: left adds, right removes.
2. **AZERTY physical keyboard defaults.** `KeyboardLayoutMode=2` automatically uses the same physical key positions on any layout; left scan 16 is A on AZERTY / Q on QWERTY, right scan 18 is E. `KeyboardLayoutMode=3` restores *literal* Q/E via the OS layout. F2–F5 still pass through.
3. **Stack wheel pulses.** Mouse scroll (8/9) is handled outside the pressed-first-frame guard which previously suppressed transitory mouse-wheel events. Visual badge reads `current/total` and scrolling triggers a small feedback label. `EnableInputDiagnostics=1` logs wheel pulses during testing.
4. **Torch (TESObjectLIGH 0x1F) classification and native action branch.** Only proven-owned, non-ambiguous items can reach the existing EquipManager call on the game's SKSE task thread. Torch is appended as a new Kind value without changing the RWL2 save wire-format. Native torch behavior still needs an in-game check.
5. **32 completely renewed independent icon medallions.** Each exact atlas tile has an independent clipping mask, transparent cell gutters, color/subject identity, and 2048×1024 BGRA8/RWAT payload. Source illustration and the deterministic atlas builder are archived; existing frame, font and plaque remain unmodified.
6. **Motion and three-wheel pager** preserved from 0.16; no new OS imports, hooks or TESV addresses.

## Ingredient limitation

`INGR` (`0x1E`) is intentionally still guarded: v0.16 game log showed `ingredient eat native route not verified`. Eating ingredients uses a distinct game-mechanic path from ALCH food and is **not** proven by the existing EquipManager wrapper. Rather than risking broken effects/inventory data, ingredient use still gives `EAT N/A`. This feature has NOT been claimed fixed.

## Verification

The full `build-v017.sh` suite passes offline: C++ type/serialization/native gate checks; strict native-preservation delta versus v0.16; new input static contracts; previous editor/time/render tests; 32-cell art QA; PE32 exports, SEH, TESV signatures. TESV.exe is user-provided locally and is not redistributed. No game executable or licensed Bethesda content is included in these archives.

The wheel navigation and torch improvements **have not yet been verified by an actual Skyrim playtest**. Only user feedback on earlier versions is game-tested.
