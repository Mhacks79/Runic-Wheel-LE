# Runic Wheel LE — v0.21.0 FORGE MASTER (private test)

Target: Skyrim Legendary Edition **1.9.32.0.8**, SKSE **1.7.3**, Windows x86/D3D9.

## Why this release is substantial

This is a consolidated feature release rather than a one-issue hotfix. v0.21 includes the v0.20 scroll-edge correction and adds reversible loadout editing **without an ESP and without changing the RWL2 co-save format**.

### Advanced editor — Inventory or Magic menu only

Open an inventory or magic menu, open the wheel with middle click, then use the following gestures while the wheel is visible. Plain left/right clicks continue to add/remove items. Outside the inventory/magic menu, left/right remain your normal equipment actions.

| Gesture | Function | Safety |
| --- | --- | --- |
| SHIFT+LMB (source), SHIFT+LMB (target) | Move the selected entry between sectors or wheels | Atomic, undoable; invalid/full/duplicate targets refused |
| CTRL+LMB (source), CTRL+LMB (target) | Copy one entry | Atomic, undoable; duplicates refused |
| SHIFT+ALT+LMB twice on different sectors | Swap the two complete sectors | Maintains both stacks and their cursors; undoable |
| SHIFT+CTRL+LMB twice | Clone an entire sector | Target MUST be empty; undoable |
| ALT+LMB | Move currently selected entry forward within its stack | Position preserved; undoable |
| ALT+mouse wheel | Reorder the selected entry backward/forward in its stack | Reversible; uses the v0.20 deduplicated detent logic |
| SHIFT+RMB / CTRL+RMB | Undo / redo | Eight-step bounded session history |
| ALT+RMB twice on same sector, within 1.75 seconds | Clear whole sector | Confirmed and undoable |
| CTRL+ALT+Q/E | Clone current wheel into previous/next EMPTY wheel | No overwrite; undoable |
| SHIFT+ALT+Q/E | Exchange whole wheels | Swap all eight sectors and per-sector cursors; undoable |
| SHIFT+ALT+RMB twice on same wheel, within 1.75 seconds | Clear entire wheel | Confirmed and undoable |

Q/E follow the **physical key positions**, so on AZERTY use **A/E**. The guide panel appears only when editing a wheel in Inventory/Magic, and can be disabled with `ShowAdvancedEditorGuide=0`.

### Additional navigation and visuals

- The three-wheel pager now visualises wheel occupancy.
- Wheel titles gain counts of entries and occupied sectors.
- Empty selected slots show a readable prompt instead of a blank label.
- `EnableQuickSectorKeys=1` enables physical top-row quick-select of sectors 1–8 (OFF by default so vanilla hotkeys are unaffected).
- Existing Nordic heraldry, eight radial sectors, arcana-school icons, next-item mini-medallions, animations and AZERTY behaviour remain intact.
- Pending drag/copy/swap operations are cancelled when the wheel closes, a menu closes or a save is loaded.
- Dynamic font buffer increased for the advanced guide (bounded to 1,400 glyphs); no new OS imports or engine detours.

## Installation / compatibility

Install the **MO2** ZIP as a standalone mod, and **disable the previous RunicWheelLE version**. The install archive contains exactly six files inside `Data/SKSE/Plugins/`: DLL, INI, four RWAT texture atlases. Tested RWL2 saves remain compatible with v0.20; the eight-step undo/redo history is session-only and is not serialized.

## Runtime status

- **No in-game test has been done of v0.21 in this environment.**
- The historical player-validated actions from v0.19 (except stack scroll) are not modified.
- The v0.20 mouse-scroll pulse fix is included, but still needs player confirmation in Skyrim.
- Consumption of raw INGREDIENTS remains deliberately disabled, pending a proven LE engine route.
- This build is **PRIVATE** and should be tried on a separate MO2 profile or backed-up save.

## Build and verification

The versioned `build-v021.sh` runs inherited quality gates, ABI/SEH/site verification, editor unit tests, **35,000 deterministic randomized operations with 2,059 serialization round-trips**, and a diff check that native equipment and co-save paths remain identical to v0.20. Sanitizer runs are separate and documented in `QA-V021.md`.
