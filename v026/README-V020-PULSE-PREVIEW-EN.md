# Runic Wheel LE — v0.20.0 FORGED NORDIC • STACK PULSE

**Private development build**, designed for Skyrim Legendary Edition 1.9.32.0.8 / SKSE 1.7.3 (Win32, DirectX 9). No ESP, no Papyrus scripts and no external UI dependency. Install the package in MO2 with the previous Runic Wheel version disabled. Existing RWL2 SKSE co-save format is unchanged.

## New since v0.19

1. **Single physical scroll notch → single stack advance**: the v0.17–v0.19 input handler could count both mouse-wheel down and zero-value release as separate detents. With two entries that returned to the same item and made the wheel appear broken. `ScrollPulsePolicy.h` now filters release/duplicate signals and supports unusual zero-value-only event feeds.
2. **Real stack QA traces**: after a successful transition, the log shows wheel, sector, `before/count → after/count` and the actual selected FormID. Raw input diagnostic logs are enabled by default in this private QA build only, and can be disabled with `EnableInputDiagnostics=0`.
3. **Next-item visual preview**: selecting a sector containing multiple entries shows a smaller medallion of the *following* entry, together with `NEXT: …` in the lower nameplate. This preview is read-only, driven by the actual selected stack index, and can be disabled with `ShowNextItemPreview=0`.
4. **Skip to nonempty sectors**: optional `PreviousOccupiedSectorScanCode` and `NextOccupiedSectorScanCode`, both disabled by default. These advance the active *sector selection* only, never equip an item. Reserved F2–F5 cannot be used. Conflicts with existing wheel/item/open/close hotkeys are ignored.
5. **Magic school highlights**: an unobtrusive colour accent per school, following the v0.19 heraldic atlas. Can be disabled with `ShowMagicSchoolAccents=0`.
6. **Existing controls unchanged**: left click equips the right hand (outside inventory); right click equips the left. Physical AZERTY/QWERTY wheel paging, mouse wheel page buttons, owned torches, spells, co-save, time slowdown and the v0.17 icon style are all preserved.

### Input notes

- A mouse-wheel down pulse (`value > 0`) performs one cycle. A subsequent zero-valued release does not cycle a second time. Very unusual zero-value-only wheel input can still cycle, with a safety interval.
- With two different items in a sector, a single notch must change `1/2 → 2/2` or `2/2 → 1/2`, including the main icon and the tiny next preview. A click must equip/use the currently displayed entry, subject to the existing native policies.
- If you experience a failure, the private build log now explicitly shows whether the pulse was accepted/rejected and what FormID was selected.

### Caveats

- The release-edge diagnosis is supported by source inspection and host-side reproduction; only a new Skyrim in-game test can confirm the exact event pattern on the user's machine.
- Eating ingredients (`INGR`) remains intentionally unavailable until a safe engine route can be verified. No new native consumption/equipment calls have been introduced.
- Artwork and screenshot previews are original, synthetic, and do not represent footage captured in Skyrim.

### Build

From the complete workstation, install the compiler recovery bundle and supply an original legally obtained TESV.exe into `inputs/TESV.exe`; run `v020/build-v020.sh OUTPUT_DIRECTORY` (Clang MSVC x86 + lld-link). The same procedure runs V015–V020 static/host checks, native preservation checks, signature gates and PE32 verification before packaging.
