# Runic Wheel LE v0.25.0 — KHA-OS window ownership & alignment

Private candidate for Skyrim LE 1.9.32.0.8 + SKSE LE 1.7.3 and KHA-OS v0.5L.

## Fixed in source

- Menu labels, +/- controls, numeric values, tabs, status footer and preview blocks use padded-font-aware vertical alignment; left field labels have their own non-overlapping region.
- Drag the modal window by its top header. It is clamped inside the game viewport.
- Close with the **X** button or **Esc**; click outside the dialog to relinquish mouse input back to the host.
- **Tab closes configuration and is forwarded to Skyrim's native menu.** It is no longer reserved for cycling KHA-OS pages; use Page Up / Page Down or click the tabs.
- Focus loss cancels the modal and yields the keyboard/mouse to Skyrim/Windows. Mouse-capture changes end a drag but do not close the entire window.
- Additional log entries track modal close/focus and drag lifetime.

## Preservation

- No change to the RWL2 save format, equipment actions, Skyrim hook addresses, KHA-OS ABI v2 or the four RWAT textures.
- 2 SKSE and 6 KHA-OS exports; x86 PE32 MSVC ABI.
- The canonical RunicWheelLE.ini remains the only configuration store.

## Limitations

This version is **not yet game-validated**. The host's exact routing of WM_MOUSEMOVE / WM_LBUTTONUP and DirectInput needs user testing with KHA-OS and ENB in Skyrim LE. A compiled DLL or passing static tests does not prove the full interaction is corrected in game.

## Test checklist

1. Open the KHA-OS Runic Wheel menu and inspect the row labels and control alignment.
2. Drag by the title bar; use the X button to close.
3. Reopen, press Tab, and confirm Skyrim's four-way menu appears and accepts clicks.
4. Reopen, press Esc, and verify the modal releases input.
5. Open the Inventory/Magic wheel; ensure existing equip and stack actions are unaffected.
