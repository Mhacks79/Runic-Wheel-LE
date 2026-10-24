# Runic Wheel LE — verified input mapping audit (2026-10-23)

## Scope / proof

Compared **uploaded original dTry Wheeler `src/bin/Config.h`, `UserInput/Controls.cpp`, `UserInput/Input.cpp`**, uploaded **Wheeler Refined 1.3.4 `SKSE/Plugins/wheeler/Controls.defaults.ini`**, official **SKSE LE 1.7.3 `skse/InputMap.cpp` / `GameInput.h`**, and **Runic Wheel LE V0.9 source + INI**. No Wheeler code or assets were incorporated in Runic. This is **code/source parity** only: no gamepad playtest exists and V0.9's inventory-hover capture is still disabled after the V0.7 crash.

The original and Refined defaults **cannot both be identical**: their bindings differ. The chosen base for our LE plugin is *dTry original*, with the user-requested middle-click override for opening and logical AZERTY/QWERTY keyboard handling.

## Original dTry ➜ target Runic LE (default)

| Function | Original Wheeler | Runic LE parity branch | Static status |
|---|---|---|---|
| Toggle/open (mouse/keyboard) | CapsLock scan 58; trigger on press, close on release after >250ms | **MIDDLE mouse button** ID 2; 260ms threshold, configurable; CapsLock unbound | User-requested override, tested state logic |
| Mouse radial aim | relative mouse delta while open | relative mouse delta | user confirmed V0.2, not yet current revision |
| Next/previous wheel | E scan 18 / Q scan 16 | logical E/Q resolved through Windows layout; also optional side buttons | static mapped, current game test pending |
| Primary/secondary | left/right mouse 256/257 | left/right mouse IDs 0/1 from raw InputEvent | V0.2 confirms click events; item equip not available |
| Next/previous item | mouse wheel down/up IDs 265/264 | raw mouse scroll 9/8 | static mapping; cycling code tested |
| Create wheel/empty entry | N scan 49 / M scan 50 | **NOT IMPLEMENTED** (catalogue fixed to three wheels and eight sectors) | gap; blocks total parity |
| Reorder entries/wheels | arrow keys (200,208,203,205) | **NOT IMPLEMENTED** | gap; blocks total parity |
| Automatic inventory edit | inventory/magic context on wheel open, mouse click to add/remove | context logic exists but hovered selection **quarantined** | **NOT FUNCTIONAL**; blocks requested parity |
| Slow down time | 0.1 game speed on world open, restore on close | 10% policy with conditional restore | source/test only, not validated in game |

## Original gamepad button translation (proof: official SKSE LE InputMap.cpp)

The original source stores **macro indices** (266–281), whereas LE `ButtonEvent::keyMask` is a **raw button mask**. Using macro indices or arbitrary XInput button masks interchangeably is wrong. Official converter `InputMap::GamepadKeycodeToMask` is authoritative for the following:

| Function | Wheeler macro | LE raw mask (hex) | Runic INI key |
|---|---:|---:|---|
| Toggle/open wheel = LT | 280 | `0x0009` (9) | `GamepadOpenMask=9` |
| Next wheel = RT | 281 | `0x000A` (10) | `GamepadNextMask=10` |
| Previous wheel = unbound | 0 | `0` | `GamepadPreviousMask=0` |
| Primary action = RB | 275 | `0x0200` (512) | `GamepadPrimaryMask=512` |
| Secondary action = LB | 274 | `0x0100` (256) | `GamepadSecondaryMask=256` |
| Previous item = D-pad LEFT | 268 | `0x0004` (4) | `GamepadPreviousItemMask=4` |
| Next item = D-pad RIGHT | 269 | `0x0008` (8) | `GamepadNextItemMask=8` |
| Aim/select | right stick, `ThumbstickEvent.keyMask==0xC` | `12` | `GamepadRightStickMask=12` |

**Confirmed V0.9 regression corrected in source:** it incorrectly used `GamepadPrimaryMask=4096` (A), `GamepadSecondaryMask=8192` (B), and `GamepadNextMask=512` (RB) while `GamepadPreviousMask=256` (LB); it also did not dispatch D-pad left/right as item cycling. All defaults and corresponding input dispatch paths have been corrected. `ControlParityTests.py` checks 18 default mappings and injects five mutations to prove regressions are detected. This is not an in-game gamepad test. LT is a special raw keyMask `0x9`, not a regular analog XInput `0x0009` bitmask in the Xbox API.

## Wheeler Refined is **not** the original mapping

Refined `Controls.defaults.ini` uses keyboard G scan 34 to open, gamepad X macro 278 to open, gamepad RT next wheel and LT previous wheel; mouse wheel up/down changes wheels, and physical Q/E changes stacked items. It also offers extra modifier and edit hints. Our default deliberately does **not** emulate this Refined preset. Any future Refined-inspired alternative should be an explicit separately named configurable profile and tested for input conflicts.

## Functional gaps and next gate

1. **No controller test in running Skyrim LE.** `GamepadOpenMask`, raw event value semantics, stick movement direction/deadzone and potential Skyrim key conflicts need game evidence, not an assertion from default values.
2. Dynamic wheel/slot creation and reordering from dTry original are not implemented. No fake handlers should claim them.
3. The inventory hover selection and actual equipment are still absent, so edit controls cannot be considered working even if button dispatch is correct.
4. RWL-001 V0.7 CTD is unattributed, so no risky Render hooks or unvalidated callbacks should be enabled to force parity.
5. Keep F2–F5 reserved, **especially F5 quicksave**, even with stale user INIs. The old F-key editor is gone.

**Conclusion:** core control identifiers and dispatch wiring now conform statically to dTry original (except the intentional middle-click/layout override), but **complete functional parity is NOT achieved and no player build is yet cleared by this audit**. Do not ask for a full original-equivalent in-game test based on this report alone.
