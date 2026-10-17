# 05: Patch our banner and save name into the disc

**What to build:** `assemble_game` writes a patched `opening.bnr` (the `BNR1` banner: ticket 04's image encoded to RGB5A3, plus its short name, maker and description) into the game folder, as it does the menu archives, so both `run` and `iso` show it ([spec](../spec.md)). `check` encodes it too, so a bad image fails there. The save comment in `src/training/new_save.c` changes from "Melee Tech Chase Training" to "RTC Training".

**Blocked by:** 04

**Status:** resolved

- [x] Dolphin's game list shows the RTC Training ISO with our banner and name (shot).
- [x] A new save shows "RTC Training" in Dolphin's memory card manager.
- [x] `orig/GALE01/files/opening.bnr` is unchanged; `dev.py check` passes.

## Answer

Resolved on branch `iso-release-05` (commit "Ticket 05: patch our banner into opening.bnr, save comment RTC Training"), built against the placeholder art from ticket 04.

- `tools/banner.py` patches a copy of the vanilla `BNR1` banner in place, keeping its size (0x1960) and layout: the 96x32 PNG `assets/banner/banner.png` encoded to RGB5A3 (opaque pixels RGB555, others A3RGB444) at 0x20, then the text of `assets/banner/banner.txt` (Windows-1252, NUL-padded): line 1 is the short name and the long name, line 2 the short and long maker, the remaining lines the description. A PNG that isn't 96x32, text that doesn't fit its field or can't be encoded, or a source that isn't a BNR1 banner is a `PatchError`.
- `assemble_game` writes `files/opening.bnr` through it (a fresh file, never a link), so `run` and `iso` both carry it; `verify_archive_patches` encodes it too, so `check`'s `archive-patches` line FAILs on a bad banner.
- `src/training/new_save.c`: the save's comment title is "RTC Training" (padded to 32 like the vanilla title). `docs/developer-workflow.md` updated (run, check, memory card snapshot); the README's banner comment is now the image.
- Tests (through the real commands): `test_iso` reads `opening.bnr` out of the ISO through its FST and checks it differs from vanilla, has vanilla's size, and decodes (with an independent RGB5A3 decoder, checked against vanilla's banner) to `banner.png` within RGB5A3 precision and to `banner.txt`'s text; the existing snapshot test covers `orig/` being unchanged. `test_check` swaps in a 95x32 PNG and expects `archive-patches: FAIL` naming 95x32 (red confirmed before the check hook was in place).
- Manual, in Dolphin: the game list (folder of the built ISO added in a throwaway Dolphin user folder) shows the RTC Training ISO with the placeholder banner, title "RTC Training" and maker "RTC Training project", beside vanilla Melee. A new save, made by `boot-new-save-to-main-menu` against a card with the `GTCE01` save removed, shows as "RTC Training / Game Data" in the GameCube Memory Card Manager. `dev.py check` passes.
