# Runic Wheel LE v0.25.1 — centered transparent rune

This private art-only revision uses the user's supplied original `user-rune.png`, replacing the former *opaque center medallion baked inside* `RunicWheelFrame.rwtx` (BGRA RWAT, 1024×1024) without touching the surrounding Nordic ring or spokes.

- Source image: `artwork/rune-replacement/input/user-rune.png` (original user-provided transparent PNG, not regenerated artwork).
- Frame baseline: `artwork/rune-replacement/input/RunicWheelFrame-v025.rwtx`.
- Generator: `artwork/rune-replacement/rebuild_frame.py` (Python 3 with Pillow and NumPy).
- Rebuild the shipped frame from the source folder using:
  `python artwork/rune-replacement/rebuild_frame.py --output RunicWheelFrame.rwtx`
- Optional reproducibility QA: from `artwork/rune-replacement`, run `python test_frame.py` after `python rebuild_frame.py`.
- Existing v0.25 DLL (KHA-OS window and input fix) is **unaltered** and is identical to the previously game-tested artifact. SKSE exports, ABI, hook logic, saves, input and INI are unchanged.
- RWL frame is a D3D9 texture mapped as a full quad centered at (cx,cy). Cropping the source by alpha rather than by canvas ensures the visible rune occupies the exact 1024×1024 atlas center, without accidental positional bias.
- Source image copyright belongs to its owner. This packaging is private. No independent in-game validation has been performed for this exact frame texture.
