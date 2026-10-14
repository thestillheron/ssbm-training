# 03: Archive patcher and the item descriptions

**What to build:** A build step that writes patched copies of the main menu's archives into the assembled game folder ([spec](../spec.md), [ADR-0007](../../../docs/adr/0007-training-build-owns-the-main-menu.md)). The archives are `MnMaAll` and `SdMenu`, both `.usd` (English) and `.dat` (Japanese). The extracted disc is read and never written, and the patched files are fresh copies, never links, as `boot.bin`'s game ID patch already is.

The patcher's interface is a source archive plus a list of replacements, and its output is the patched archive. A replacement is either:
- a texture, found by its image descriptor and replaced from a committed image, encoded to the target's format (IA4 and I4 at least);
- an SIS string entry, replaced with new text.

Pixel and string data are overwritten in place, so offsets, relocations and every other byte stay as they were. A replacement image whose size doesn't match the target, or a string too long for its slot, is a build error, never resized or truncated.

The first visible use is text: the main menu's description box shows "Learn to tech chase on reaction, one step at a time." for RTC 101 and "Set up a drill of your own: options, stage, layout, vanish." for Custom Drill, in both language variants. The description entries to replace follow the item order ticket 01 settled. `ref/main-menu/` (gitignored) has archive and texture decoding scripts and the retail art to start from. The main menu's description indices are 0x81–0x85, in `SIS_MenuData`.

**Blocked by:** 01

**Status:** resolved

- [x] `tools/dev_tests` tests, on a copy of a real archive:
  - an IA4 and an I4 texture replacement decodes back to the input image;
  - a replaced string reads back;
  - every byte outside the replaced data is unchanged;
  - a wrong-sized image and an over-long string are errors;
  - `orig/` is never written.
- [x] `dev.py build` writes the patched archives into the game folder as regular files, and the extracted disc's copies are unchanged.
- [x] The description box shows the new text with the cursor on RTC 101 and on Custom Drill (shots), and Options and Data keep theirs.
- [x] After switching Options' language to Japanese, the description box still shows the new English text for our items (shot).
- [x] `dev.py check` passes.

## Comments

- **API** (`tools/archive_patcher.py`): `patch_archive(src, dest, replacements)` reads `src`, writes `dest` as a fresh regular file (an existing link at `dest` is replaced, never followed; `src` as `dest` is an error) and raises `PatchError` before writing anything if a replacement doesn't fit. `TextureReplacement(descriptor, image)`: `descriptor` is the image descriptor's offset in the archive's data section (what `ref/main-menu/dump.py` prints as `@0x...`; it differs between `.usd` and `.dat`), `image` is an `Image` or a PNG path (`read_png`, 8-bit non-interlaced). The image must equal the descriptor's width and height. Formats I4, I8, IA4, IA8; intensity comes from the RGB (alpha is ignored for I4/I8, 16 levels each way), alpha from the alpha channel. Only image pixels are written, so tile padding bytes keep theirs. `StringReplacement(index, text, template=None, spill=None)`: `index` is the SIS_MenuData entry, `
` separates lines, ASCII letters/digits/space and `,.:!?` only (the same glyph codes in both fonts; the Japanese font's `-` and `'` differ, so they're refused). `read_sis_lines(archive, index)` reads entries back.
- **Declaring replacements**: `tools/archive_patches.py` has `ARCHIVE_PATCHES`, file name under `files/` to its replacement list (`MnMaAll.usd`/`.dat` are empty now, `SdMenu.usd`/`.dat` have the two descriptions). `assemble_game` in `dev.py` applies them, so it runs in `dev.py run` (the game folder is assembled at launch, not by `dev.py build`), as fresh copies; a `PatchError` stops the run with the file name. To replace a texture: find the descriptor offset in each variant (`py ref/main-menu/dump.py orig/GALE01/files/MnMaAll.usd MenMainCursor_Top mat` lists the label frames, e.g. 176x30 IA4 at `@0xbffa8`.. in `.usd`; the `.dat` offsets are different), then add `TextureReplacement(0x..., ART / "label_rtc_101.png")` to both lists (`ART` is `assets/main-menu`). Add a scenario shot to check it.
- **Why descriptions spill.** A retail description slot holds 34 to 56 bytes (two bytes a letter) and ours need about 110 as two lines, so each text is built on retail's two-line entry 0x8D (`template=("SdMenu.usd", 0x8D)`, since the Japanese archive has no two-line description of its own) and moved into the space of unreachable descriptions: 0x81 into 0x86-0x88, 0x82 into 0x89-0x8C (the 1-P Mode and VS. Mode submenus' descriptions). The entry's table pointer and the donors' pointers are repointed to the new text; nothing else in the archive changes. Ticket 05's other descriptions (if any) need other unused entries.
- **Shots** are in the session scratchpad under `shots/03/` (`desc-*`, `japanese-*`). The new scenarios `main-menu-descriptions` and `main-menu-japanese` reproduce them; the Japanese one needs a stick-left to pick Japanese and presses A twice (pick, then confirm), and the language is saved to the run's copy of the card only.
