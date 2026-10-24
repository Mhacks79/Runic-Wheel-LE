# Architecture and engineering boundaries

Runic Wheel LE is a native SKSE 1.7.3 plugin targeting the original 32-bit Skyrim engine.

## Main subsystems

1. **SKSE lifecycle** — plugin query/load, version checks, per-save serialization, game/UI task scheduling.
2. **Engine-facing actions** — guarded equipment and consumption routes, explicit fail-closed handling of unsupported instances.
3. **Catalog and co-save model** — three pages, eight sectors per page and four entries per sector (RWL2 typed bindings).
4. **Input** — physical scan codes for AZERTY/QWERTY, mouse hold/click/scroll, optional gamepad and editor gesture policies.
5. **Graphics** — original D3D9 presentation, independently generated BGRA/RWAT atlases and state restoration.
6. **Configuration** — INI-backed options, appearance/layout/navigation settings and profiles.
7. **KHA-OS optional UI** — host API v2 exports; does not share rendering context ownership with the game wheel.
8. **Offline policy and regression checks** — distinguish testable algorithms from game-memory hooks.

## Important invariants

- No SE/AE offsets or ABI assumptions.
- Game operations must execute on appropriate SKSE thread/task paths.
- D3D9 hooks must preserve the host device state.
- Never assume FormID uniquely identifies an enchanted or customized item instance.
- A hidden page or sector retains saved contents; visible capacity cannot exceed the fixed 3 × 8 × 4 model.
- Releasing or losing input focus must not trap Skyrim's UI.
- Ingredients are used through the same EquipManager call as the vanilla inventory, never a guessed route.
- Reusable engine discoveries belong in CommonLibLE and Knowledge within the **private laboratory**, not in this public repo with proprietary executable evidence.

## Maintenance status

Current game-tested baseline reported by the author: **v0.25.1**. v0.26.0 adds exact item copies, shouts, powers, scrolls and ingredients, and isolates its drawing from post-process mods; it has not been tested in game yet. Coverage across all mod configurations is not claimed.
