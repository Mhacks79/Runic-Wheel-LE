# Runic Wheel LE v0.23.0 (private) — legible, inventory-only advanced editor

Target: Skyrim Legendary Edition 1.9.32.0.8, SKSE 1.7.3, 32-bit D3D9.

Changes from v0.22:

- The right-side controls panel only appears while **Inventory or Magic** is open **and** the advanced editor is enabled and the wheel is visible. It does not appear during ordinary world gameplay. An old `ShowControlsGuide=1` option is deliberately ignored.
- Larger, proportional fonts and row spacing, with two readable columns (physical key gesture / action) instead of 13 tiny centered lines.
- Responsive layout at 800×600, 1024×768, 1280×720, 1600×900, 1920×1080, 2560×1440 and ultrawide. On very small viewports the guide must overlap part of the radial wheel because there is no physical side margin.
- Highlighted status for pending move, copy, swap, clone or clear confirmations is preserved.
- Engine equip/unequip/consume functions, serialization (RWL2), item stacking and late-rendered mini-medallions were not changed.
- No ESP, no third-party assets; 6 required files in the MO2 package.

Testing: static/host regressions and Win32 x86 compilation passed. D3D9/ENB visibility of the new layout is not yet validated in game.

Install: disable the previous Runic Wheel version in MO2 and enable this archive in the same test profile; do not enable both. Existing per-character SKSE co-saves are preserved.
