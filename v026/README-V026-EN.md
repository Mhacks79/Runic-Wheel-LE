# Runic Wheel LE v0.26.0

Built on the v0.25.1 source. Co-save format, wheel model, input, editor, artwork and the
KHA-OS settings are unchanged; the save record is byte-for-byte the v0.25.1 one, so a
v0.25.1 save loads as it is.

## What could be put on the wheel but not used

| Item | v0.25.1 | v0.26.0 |
|---|---|---|
| Tempered, player-enchanted or renamed weapons and armor | refused, and so was **every** copy of that item as soon as you owned one modified copy | the wheel keeps the exact copy you hovered and equips that one |
| Shouts | refused every time | equipped through the game's own shout route, after checking the shout is learned and its first word unlocked; refused while shouting, like the favorites menu |
| Powers and lesser powers | sent to a hand | sent to the voice slot, the way the Magic menu does it |
| Scrolls | could be added, then refused | equipped to the hand you click with |
| Ingredients | refused | eaten, through the same call the inventory uses |
| Books, keys, soul gems, misc items, abilities | could be added, then refused | can no longer be added |

How the exact copy works: when you add a weapon or a piece of armor, the wheel stores the
item ID SKSE gives that inventory row (the same one SkyUI's favorites use). When you use
it, the wheel looks through your copies of that item for the one with that ID. If it is
gone (sold, dropped, renamed), the wheel refuses instead of grabbing another copy.
Entries made with older versions have no ID: they take an ordinary copy first, and a
modified one if that is all you have.

Items and spells from any mod work the same way as vanilla ones: everything goes through
the item's own form and type, nothing is tied to Skyrim.esm.

## Display with Aether Reloaded, ENB and other post-process mods

The wheel and its KHA-OS panel are drawn at Present, after every other pass. A
post-process mod can leave the graphics card in a state the wheel never expected:

- sRGB writes or sRGB texture reads left on: washed-out or too dark wheel;
- a colour-write mask, blend operation or separate alpha blend left changed: parts missing;
- a texture transform left on: shifted artwork;
- a smaller viewport left from a shadow or effect pass: wheel shrunk, moved or cut;
- an extra render target left bound: nothing drawn at all.

v0.26 sets every one of those itself, lays the wheel out on the real back-buffer size and
unbinds extra render targets for the draw. All of it is put back afterwards exactly as it
was.

The KHA-OS panel can also lose its mouse pointer when another overlay's interface hides
the Windows pointer. The panel now draws its own pointer while the Windows one is hidden
(`KhaosCursorMode` in `RunicWheelLE.ini`: 0 off, 1 automatic, 2 always), and mouse
positions are mapped to the back-buffer when the window and the back-buffer differ in size.

## Build

`build-v026.bat` (Microsoft C++ x86, static runtime, real SEH). Set `SKSE_LE_SDK` to the
SKSE 1.7.3 source folder first. The DLL exports the two SKSE entry points and the six
KHA-OS ones, and imports only kernel32 and user32.

## Checks run offline

Host unit tests (23 programs, checks that stay on in release builds), the v0.15-v0.25
contract scripts updated where v0.26 changes behaviour on purpose, `V026Contracts.py`
(every engine site read back from TESV.exe 1.9.32.0.8, co-save frozen to v0.25.1, SKSE
CRC table equal to the one the wheel uses), and six negative controls that put a bug back
and must fail.

## Not tested in a game

Everything above. In particular: exact-copy equipping, shouts, powers, scrolls,
ingredients, the Aether display and the KHA-OS pointer have not been seen running.
A light plugin whose load order changes inside a playthrough keeps its items but the
exact copy of a modified weapon from it may then be refused (the stored ID is tied to
the item's ID at the time it was added).
