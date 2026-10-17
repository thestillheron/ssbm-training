# 01: `dev.py iso` builds the RTC Training ISO

**What to build:** A `dev.py iso` command ([spec](../spec.md), [ADR-0008](../../../docs/adr/0008-players-get-a-patch-not-the-game.md)) that builds the training build without authoring tools, assembles the game folder with `assemble_game`, and packs it into `build/training/rtc-training.iso` with gc_fst `rebuild`, then `set-header` to `GTCE01` and "RTC Training". gc_fst is fetched by `tools/download_tool.py`: Windows and Linux from gc_fst's GitHub releases, macOS from TM-CE's `bin/gc_fst_macos_arm64` at a pinned commit. Check first that `rebuild` accepts our `sys/` + `files/` layout; if not, record what it needs here. Documented in `docs/developer-workflow.md` under Commands.

**Blocked by:** None (can start immediately)

**Status:** resolved

- [x] `dev.py iso` on a clean checkout downloads gc_fst and writes `build/training/rtc-training.iso`; re-running reuses the download.
- [x] `dtk disc info` on the output shows game ID `GTCE01` and title "RTC Training".
- [x] The ISO boots in Dolphin to the main menu with "- T" (ask the user before launching Dolphin; `dev.py stop` or close it afterwards).
- [x] Files under `orig/` are unchanged.
- [x] `dev.py check` passes.

## Answer

`dev.py iso` is in (branch `iso-release-01`), tested by `tools/dev_tests/test_iso.py`, which drives the real command in a sandbox copy of the repo and skips without a disc image, and documented under Commands in `docs/developer-workflow.md`.

**gc_fst `rebuild` does not accept our `sys/` + `files/` layout** (checked in gc_fst's `src/lib.rs`, `write_iso`). It reads `<root>/&&systemdata/ISO.hdr` (the first 0x2440 bytes of the disc: our `boot.bin` (0x440) followed by `bi2.bin` (0x2000)), `&&systemdata/AppLoader.ldr` (our `apploader.img`) and `&&systemdata/Start.dol` (our `main.dol`), and packs every other file and folder under `<root>` as the disc's filesystem, writing its own FST (so `fst.bin` isn't needed). Given our game folder as is, it would put `sys/` and `files/` into the disc's filesystem. So `iso` hard-links the assembled game folder into that layout in `build/training/iso-root/` (writing `ISO.hdr` fresh from the game folder's `GTCE01` `boot.bin`) and removes it after `rebuild`. It then runs `set-header <iso> GTCE01 "RTC Training"`.

gc_fst is pinned at `v2.1` (`GC_FST_TAG` in `dev.py`; release assets `gc_fst.exe` and `gc_fst`). On macOS `tools/download_tool.py` fetches `bin/gc_fst_macos_arm64` from TM-CE at commit `16fc45650f6036b1445dffce403eff9d353b310e`.

Verified on Windows:
- From a clean sandbox, `iso` downloads gc_fst and writes `build/training/rtc-training.iso` (1,459,978,240 bytes); a second `iso` doesn't download it again.
- `dtk disc info` on it: `Title: RTC Training`, `Game ID: GTCE01`.
- Every file under the sandbox's `orig/GALE01` has the same SHA-1 before and after.
- Boot in Dolphin (`Dolphin -e rtc-training.iso`, with a movie of `start` presses against the memory card snapshot): the title screen shows "- T", and the main menu shows RTC 101, Custom Drill, Options and Data. Closed with `dev.py stop`.
- `dev.py check` passes.

Not verified: macOS (the arm64 URL wasn't downloaded or run here), and the game list's banner and name (that's the banner ticket's job). `dev.py pad` keystrokes didn't reach the game when it was booted from the ISO without a movie, so the boot check used a movie. That may be focus and not the ISO, but nobody checked.
