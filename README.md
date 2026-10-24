# Runic Wheel LE

**A native Nordic radial equipment and magic interface for Skyrim Legendary Edition.**

**Runic Wheel LE** is an independently developed SKSE plugin for **Skyrim Classic / Legendary Edition (TESV.exe 1.9.32.0.8)**. It delivers an original Direct3D 9 radial interface for equipment, spells, shouts, powers, consumables, and per-character loadouts.

> **Original work by Mhacks79.** Inspired by the *concept* of [Wheeler – Quick Action Wheel Of Skyrim](https://www.nexusmods.com/skyrimspecialedition/mods/97345) by dTry, but **not a Wheeler port or fork**. No Wheeler code, DLL or assets are included or claimed.

[![Target](https://img.shields.io/badge/Skyrim-LE%201.9.32.0.8-78654c)](https://www.nexusmods.com/skyrim)
![SKSE](https://img.shields.io/badge/SKSE-1.7.3-478ba2)
![Platform](https://img.shields.io/badge/Platform-Win32%20%2F%20Direct3D9-474e59)
![License](https://img.shields.io/badge/Source%20license-All%20Rights%20Reserved-c25e4e)

[Support development on Ko-fi](https://ko-fi.com/mhacks79)

## Highlights

- Native **Win32/x86 SKSE 1.7.3** plugin, **no ESP** required.
- Up to **3 pages × 8 sectors × 4 items** per sector (96 entries). Reducing visible pages/sectors does not remove stored assignments.
- Independent Nordic wheel artwork, distinct heraldic equipment icons and separate magic-school, shout and power medallions.
- Inventory/Magic assignment, stack scrolling and previews, contextual editor, undo/redo and advanced sector/page management.
- Customizable visual scale, placement, captions, palettes, input bindings and time slowdown.
- Optional **KHA-OS v0.5L** configuration menu; the wheel runs without KHA-OS.
- Uses SKSE co-save serialization for wheel assignments and an INI for settings.
- Transparent central Nordic rune, original texture resources.

## Requirements

| Component | Version |
|---|---|
| Skyrim Legendary Edition / Classic | **1.9.32.0.8** |
| SKSE LE | **1.7.3** |
| Platform | Windows, 32-bit Skyrim, Direct3D 9 |
| KHA-OS | Optional compatible v0.5L host |

**Not compatible** with Skyrim Special Edition or Anniversary Edition DLL loading.

## Installation

Install the compiled mod archive with Mod Organizer 2, preserving the supplied `Data/SKSE/Plugins/` paths. Launch Skyrim through SKSE. Open Inventory or Magic, hover an entry and use the default middle mouse button to configure the wheel. Gameplay controls and shortcuts are described in [docs/CONTROLS.md](docs/CONTROLS.md).

> This repository is intended for **source inspection and development**, not distribution of proprietary Bethesda executables or the private reverse-engineering laboratory. Obtain release binaries from the authorized mod distribution page.

## Source publication status

**Repository import in progress.** The documentation and selected original policy headers are published. The rest of the v0.25.1 C++ implementation, tests and artwork have been prepared as a separate Git-ready source package; the live repository is **not yet a complete source checkout**. Do not assume that cloning this repository currently produces a buildable DLL. See [SOURCE-STATUS.md](SOURCE-STATUS.md).

## Source and building

See **[docs/BUILDING.md](docs/BUILDING.md)** for the native x86 build, host prerequisites, SDK boundary and verification procedure; **[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)** for the module design; and **[docs/THIRD-PARTY.md](docs/THIRD-PARTY.md)** for provenance.

Offline test success and a valid PE32 DLL **do not imply validation in game**. Test new builds in an isolated mod profile and back up saves.

## Current limitations

Raw ingredient consumption from the wheel is deliberately unsupported pending a verified native engine route. Customized item-instance selection is guarded rather than guessed. Changing page/sector visibility does not expand the fixed co-save model.

## Source-available, not open source

**Copyright © 2026 Mhacks79. All rights reserved.**

The original Runic Wheel LE implementation is made available, where published, **for viewing and reference only**. No license is granted to copy, adapt, redistribute, republish, build derivative projects from, or reuse the code or original assets without **prior written permission** from Mhacks79. Reading code on GitHub does not grant a reuse license. See [LICENSE](LICENSE) and [docs/THIRD-PARTY.md](docs/THIRD-PARTY.md). Third-party components remain subject to their own licenses.

For permissions, issues and provenance, consult [CONTRIBUTING.md](CONTRIBUTING.md). Report security-sensitive findings privately; see [SECURITY.md](SECURITY.md).

---

**Created by [Mhacks79](https://github.com/Mhacks79) · [Support on Ko-fi](https://ko-fi.com/mhacks79)**

*Forged for Classic. Built to last.*
