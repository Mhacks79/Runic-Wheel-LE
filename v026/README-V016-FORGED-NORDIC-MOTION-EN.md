# Runic Wheel LE 0.16.0 — Forged Nordic Motion (PRIVATE)

For **Skyrim LE 1.9.32.0.8 / SKSE 1.7.3**, Win32 x86. Use in a disposable save and separate MO2 profile. Tested OFFLINE only; actual in-game D3D9/ENB behavior requires confirmation.

## Changes since the player-tested v0.15

- Opacity and restrained 0.966-to-1.0 scale on entry (160ms), independent of FPS and game time.
- Selected sector glow crossfades between the previous and next sector (125ms); direct 8-to-1 transition, never rotates through other slots. Gameplay selection stays immediate.
- Three engraved Nordic indicator diamonds outside the south-east rim display which wheel is active.
- Click result glints and nameplate feedback fade gracefully, without pretending an unevaluated action succeeded (existing `EQUIP?`, `USE?`, `UNEQUIP?` labels remain conservative).
- On release/escape, inputs close immediately; an optional 110ms visual-only exit fade does **not** retain input capture or intentionally delay time restoration. Focus loss, load/reset and render fault skip the fade.
- All four original RWAT atlases remain exactly those of v0.15. Geometric fallback retained. No new engine hooks, TESV addresses or changes in `perform_action`.

## INI additions ([Controls])

```
EnablePresentationAnimations=1
ReduceMotion=0
OpenAnimationMs=160
CloseAnimationMs=110
SectorTransitionMs=125
```

All values bounded when read. Set `EnablePresentationAnimations=0` for old instant appearance; `ReduceMotion=1` suppresses scale and selected-sector movement but keeps fades. Any duration can be set to `0`. Existing wheel scale, opacity, feedback duration, ENB state block and all original binds remain intact. F2–F5 pass through unchanged.

## Validation

`build-v016.sh` runs previous visual/native/offline tests, v0.16-specific integration tests and a compiled deterministic animation test covering counter rollover and the lifecycle. The resulting DLL must pass `verify-v011.py` for SKSE exports, Win32, SEH and TESV signatures.

**Not in scope:** new routes for real dragon shouts, per-instance enchanted items or ingredients. Their explicit safety limitations have not been changed.
