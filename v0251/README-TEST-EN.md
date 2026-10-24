# Runic Wheel LE v0.12.1 — Private readability / inventory diagnostic

This is a private diagnostic build, **not a release**. Tested offline only. Keep v0.12.0 as the game-tested rollback.

## Changes

- Emblem panels enlarged to 70×67px, category accents/shadow, pictograms enlarged 33% and per-sector item names enlarged. The selected-name caption and click notification are larger. No external assets used.
- Player inventory count checks include `TESNPC::container` base inventory (offset 0x58) plus `ExtraContainerChanges::countDelta`, matching SKSE LE's count model. A shield available only from base inventory may previously have been refused. Does not bypass special/ambiguous item checks.
- Diagnostics for rejected actions now include FormID and TESV FormType, allowing shield vs ingredient failures to be attributed.
- Ingredients (form type 0x1E) are now recognized for catalogue/display but intentionally **not eaten**: `EAT N/A` is shown, because their actual native usage route is not verified. Do not confuse it with a fixed ingredient action.
- Equipment/unequipment and spell native calls retained from the player-tested v0.11.2 route. No original Wheeler assets/source copied.

## Test on a disposable save

1. Disable all other Runic Wheel DLL copies in MO2; install this ZIP. Keep the previous co-save and make a backup.
2. Open the wheel, assess emblem size, sector names and selected-item caption at your resolution.
3. Add/select a plain shield from Inventory. Try the equip/unequip toggle, and note whether the response matches.
4. Add an ingredient from Inventory. The entry should display but attempting to use it intentionally shows `EAT N/A`.
5. Check a familiar ordinary item and known spell, slowed time, F5 quicksave, and the save/reload persistence.
6. If anything regresses, remove this version and reinstall v0.12.0 (or v0.11.2), reload your disposable save, and share the RunicWheelLE.log.

**Important:** native inventory reads changed. Even verified SKSE offsets and successful tests cannot certify game compatibility. Do not test on the main save. This is not a general-purpose ingredient-consumption release.
