# 02: `dev.py release` makes the release patch zip

**What to build:** A `dev.py release` command ([spec](../spec.md)) that runs ticket 01's ISO build and encodes a [release patch](../../../docs/glossary.md#glossary_release_patch) against the developer's vanilla disc: converted to a plain ISO with `dtk disc convert` if it isn't one, and checked with `dtk disc verify` first. It writes `build/release/rtc-training.zip` holding a `rtc-training/` folder with `patch.xdelta`, `xdelta3.exe`, `DRAG VANILLA MELEE HERE.bat` and `build_linux_and_mac.sh`, modelled on TM-CE's `TM-CE/` folder, writing `rtc-training.iso` next to the scripts. Developer-side xdelta3: downloaded on Windows; on macOS `setup` checks for it and prints `brew install xdelta` if missing. Documented in `docs/developer-workflow.md`, including the manual upload to GitHub Releases with a `vX.Y.Z` tag.

**Blocked by:** 01

**Status:** resolved

- [x] The zip's `.bat`, given a copy of the vanilla ISO, writes an ISO byte-identical to `build/training/rtc-training.iso`.
- [ ] The `.sh` does the same on macOS (or Linux), with xdelta3 installed.
- [x] The `.bat` given an RVZ or a modified ISO prints the "not a valid v1.02 NTSC melee iso" error.
- [x] Release builds never include the authoring tools or a non-default tech pool, whatever `build/dev_state.json` was configured for.
- [x] `dev.py check` passes.

## Answer

`dev.py release [DISC]` is in (branch `iso-release-02`), tested by `tools/dev_tests/test_release.py` (drives the real command in a sandbox copy, skips without a disc image) and documented under Commands in `docs/developer-workflow.md`, with a "Publishing a release" section for the manual `vX.Y.Z` upload. The scripts live in `tools/release/` (`.gitignore` ignored `*.bat`, so it now excepts that folder).

**The disc in `orig/GALE01/` here is an NKit image** (`NKIT v01` at 0x200, 1,430,679,552 bytes instead of 1,459,978,240). `dtk disc verify` says "partial match": CRC32 `5365c84b` matches (NKit keeps the original's CRC) but SHA-1 `6e83...bef5` isn't Redump's `d4e70c06...26fc`. So `release` refuses it, as the spec wants, and the maintainer needs a clean dump (or the NKit restored to a full ISO) to publish (ticket 06). Also, `dtk disc verify` exits 0 on a mismatch, so `release` reads its report: it needs `Redump: Super Smash Bros. Melee (USA) (En,Ja) (Rev 2) ✅` and no ❌.

Because of that, `release --allow-unverified-disc` exists for testing: it goes on with a warning and writes `build/release/rtc-training-UNVERIFIED.zip`. The tests fall back to it when the disc fails verification; every check after the gate was run that way here. A side effect of the NKit disc: the RTC Training ISO inherits the `NKIT v01` bytes from `boot.bin` (Dolphin's DolphinTool calls it an NKit file). That goes away with a clean disc; nothing in `iso` changes for it.

xdelta3 is jmacd/xdelta `v3.2.2` (Apache-2.0, `XDELTA3_TAG`), its `windows-x86_64` zip downloaded by `tools/download_tool.py` (`xdelta3-windows`) on every OS, because the exe goes into the zip. Encoding uses `-B` as large as the disc (the default 64 MB window gave a 1 GB patch; with it the patch is about 830 KB, the zip about 850 KB) and `-S none`. 3.2.2 also writes a BLAKE3 of the source into the patch and checks it, so the zip's `xdelta3.exe` rejects any other source outright; xdelta3 3.0.11 and 3.1.0 (the Debian/Ubuntu and Homebrew versions) apply the patch byte-identically too, and fail on a modified ISO with "target window checksum mismatch".

Verified on Windows (test suite, plus by hand):
- The zip holds exactly `rtc-training/` with `patch.xdelta`, `xdelta3.exe`, `DRAG VANILLA MELEE HERE.bat` (CRLF) and `build_linux_and_mac.sh` (LF, mode 755).
- Applying the patch to a copy of the vanilla ISO gives a file byte-identical to `build/training/rtc-training.iso`; applying it to a modified copy fails.
- After `build --authoring --tech-pool in`, the ISO `release` makes is the one a following `iso` makes (no authoring tools, default pool).
- The `.bat`, run as a drag and drop does (`cmd /c`, from another folder) with a copy of the vanilla ISO, writes `rtc-training.iso` beside itself, byte-identical; with a modified ISO, and by hand with an RVZ, it prints "ERROR: The ISO is not a valid v1.02 NTSC melee iso" and leaves no output.
- The `.sh` under Git Bash (with the zip's `xdelta3.exe` on `PATH`) does the same, and fails the same way on a modified ISO.
- A modified disc is refused before anything is built; files under `orig/GALE01` and the source disc are unchanged.
- The RVZ path: an RVZ is converted with `dtk disc convert` into `build/release/vanilla.iso` (deleted afterwards) and the patch applies to the plain ISO. Tried with an RVZ of a non-NKit ISO; dtk can't read an RVZ made from the NKit disc.
- `dev.py check` passes.

Not verified: the `.sh` on real macOS or Linux (no WSL distro with xdelta3 here), the macOS `setup` xdelta3 message and the Homebrew encoder, and a release from a disc that passes verification (none available).
