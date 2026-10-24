# Runic Wheel LE v0.19.0 — Forged Nordic Arcana & Stack Navigation

This is a private Skyrim Legendary Edition 1.9.32.0.8 / SKSE 1.7.3 / D3D9 development build. Install only one version at a time in MO2.

* Canonical stack cursor bug fixed; visual item, subtitle, badge and equipped FormID use the same selected entry.
* Three optional personalized wheel headers and selected stack progress, with 720p clipping guard.
* Optional keyboard shortcuts for stack cycling (disabled by default), plus scroll reversal.
* Five magic schools have individual Nordic medallions and contextual labels; Powers and Dragon Shouts are distinct.
* Spell visual classification checks bounded readable effects; native gameplay actions are deliberately unchanged.
* Art can be regenerated from bundled V017-Heraldry-Base.png using `python art/v018_arcana.py`.
* Ingredients remain guarded (no native eating route established).
* Source and executable pass offline unit/static/Win32 checks; NOT validated in the Skyrim game for this version.

Configuration keys: ShowWheelTitles, Wheel1Title, Wheel2Title, Wheel3Title, ShowStackHints, PreviousItemScanCode, NextItemScanCode, ReverseItemScroll. See RunicWheelLE.ini.
