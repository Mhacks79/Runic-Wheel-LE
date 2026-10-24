# Release qualification checklist

- [ ] Review `git diff` for TESV.exe, game dumps, credentials, SDKs, unrelated mods and temporary files.
- [ ] Confirm ownership and redistribution rights for each submitted source/art asset.
- [ ] Verify clean compilation for x86 PE32 with exact SKSE 1.7.3 ABI.
- [ ] Check two SKSE exports; optionally six KHA-OS exports; import tables and SEH assumptions.
- [ ] Check all new engine signatures against the exact TESV.exe build.
- [ ] Run policy unit tests, negative tests and source-preservation regression tests.
- [ ] Check all resource dimensions, transparent gutters and proper atlas mappings.
- [ ] Confirm INI options, changed defaults and backward-compatible co-saves.
- [ ] Exercise input/focus restoration, Inventory/Magic and game contexts.
- [ ] Test ENB + non-ENB if possible; never claim compatibility without observed evidence.
- [ ] Record binary and source SHA-256; test the exact artifact intended for users.
- [ ] Update private CommonLibLE and Knowledge for new reusable technical facts.
- [ ] Distribute game-ready assets through the intended release channel, not source repository.
