# Runic Wheel LE v0.13 — Original first-concept icon integration (PRIVATE VISUAL TEST)

**Player-selected art direction:** the **very first approved concept**, not either of the two later ice-blue or amber-dragon variants. Dark charcoal/blackened steel, engraved antique gold, translucent smoky segments, elegant Nordic runes, warm selection highlights and large illustrated item medallions.

**Stage delivered:** 16 original illustrated circular medallions converted into a 1024² premultiplied-free RGBA/BGRA image atlas; DirectX 9 `D3DPOOL_MANAGED` texture with safe fallback to v0.12.1 procedural pictograms if art is missing or fails to load. Enlarged icon artwork replaces the 9×9 geometric category glyphs in each populated sector, retaining readable item labels. The existing flat wedge geometry receives a warmer palette. The texture is derived from the first concept-sheet generated specifically for this project; no Wheeler source or imagery used.

**NOT YET delivered:** the complete photorealistic ornate wheel rim, high-quality scalable typeface, reactive glow/metal animations, exact individual weapon/shield/ingredient FormID icon identification, and special enchantment icons. Those require a separate, game-validated renderer pass and subtype-safe game-thread data. Do not present this test as a pixel-perfect match to the artwork mockup.

## Install for private test

1. In MO2 disable v0.12.1, enable ONLY this package containing three files in `Data/SKSE/Plugins`: `RunicWheelLE.dll`, `RunicWheelLE.ini`, `RunicWheelIcons.rwtx`.
2. Launch Skyrim LE 1.9.32.0.8 + SKSE 1.7.3 with a disposable save. Open the wheel with middle mouse and verify sword, armor, spells, potions and icon legibility. Try save/reload, inventory add, spell action, time slowdown and controller if present.
3. In the diagnostic log expect `first-concept game icon atlas READY (16 original medallions)` after wheel first opens.
4. If art fails to load, the mod should still display the previous pixel emblems. Set `ShowConceptIcons=0` to force that baseline presentation and restart Skyrim. If any actual equipment regression occurs, disable this build and return to the player-verified v0.12.1.

## Safety and provenance

- Native equip, spell, ingredient-fail-closed, co-save and SKSE input routines are copied from v0.12.1 and have not been modified in v0.13. INI path construction was corrected because the earlier copy omitted the directory prefix; confirm custom INI values now take effect.
- The atlas is a 12-byte little-endian `RWAT` header (`RWAT` + width/height u32) followed by uncompressed BGRA 1024×1024, stored outside the DLL. Texture data is never interpreted as a TESV pointer. Missing/invalid atlas fails closed to old glyphs.
- Fixed function FVF/texture-stage settings are restored for subsequent inline geometry and the enclosing D3D9 state block handles the wider render-state rollback. No external graphics runtime requirement.
- The reference images are *art direction references*, not actual in-game screenshots, and cannot directly serve as ready-to-play radial-wheel UI textures. Original AI-generated material from this project is cropped and adapted into game sprites; no copyrighted game-mod assets copied.
- The test has been statically verified and compiled; visual behavior and safety must still be confirmed in the user’s Skyrim session before public release.
