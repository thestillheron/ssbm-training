# 03: README for players

**What to build:** Replace `.github/README.md` (upstream's decomp README) with a page for players ([spec](../spec.md)): banner image (ticket 04's art, once it exists), one-line summary of [RTC Training](../../../docs/glossary.md#glossary_rtc_training), a link to the latest release, how to apply the patch on Windows (drag the ISO onto the `.bat`) and macOS/Linux (install xdelta, run the `.sh`), converting RVZ to ISO in Dolphin first, what's in it (RTC 101, the custom drill, drill history), its own save that never touches a vanilla Melee save, "tested on Dolphin", bug reports through GitHub Issues, a "Building from source" section pointing at `docs/developer-workflow.md` and `dev.py iso`, a link to upstream [doldecomp/melee](https://github.com/doldecomp/melee) for the decomp it's built on, and a link to `credits.md` (which already credits TM-CE). Modelled on TM-CE's README. Add to `docs/developer-workflow.md` that upstream syncs keep our README on conflict.

**Blocked by:** 02 (file names and steps); the banner image can be added after 04

**Status:** resolved

- [ ] Every step in the README, followed by someone with only Dolphin, a vanilla ISO and the zip, ends at RTC Training's main menu.
- [ ] Every link resolves (release link once the repo is public).
- [ ] Nothing in it is upstream's decomp setup.

## Answer

`.github/README.md` (the copy GitHub shows on the front page) is now a player page, on branch `iso-release-03`, modelled on TM-CE's: one-line summary, a download link to `/releases/latest` (`rtc-training.zip`), what's in it (RTC 101, Custom Drill, drill history and its graph), the separate save (`GTCE01`), "tested on Dolphin" (console untested), what you need (Dolphin and a plain NTSC 1.02 ISO), what isn't supported (PAL, JP, other revisions, modded ISOs and NKit images, including an RVZ made from one, as ticket 02 found), RVZ-to-ISO conversion in Dolphin (Convert File..., Format ISO), the Windows steps (`DRAG VANILLA MELEE HERE.bat`), the macOS/Linux steps (install xdelta, run `build_linux_and_mac.sh path/to/melee.iso`), opening `rtc-training.iso` in Dolphin to the main menu, bug reports through GitHub Issues, a short "Building from source" section (built on doldecomp/melee, `dev.py setup` and `dev.py iso`, link to `docs/developer-workflow.md`) and a link to `credits.md`. None of upstream's decomp setup (ninja, objdiff, Discord, CI badges) is left.

The banner is an HTML comment at the top, holding the `<img src="../assets/banner/banner.png">` line to use once ticket 04 commits the art, so no broken image shows until then. Relative links in `.github/README.md` resolve from `.github/`, so they are `../credits.md`, `../docs/developer-workflow.md` and `../assets/...` (upstream's README used `assets/` for `.github/assets/`).

`docs/developer-workflow.md` has a new "Merging upstream" section: on conflict keep ours (`git checkout --ours .github/README.md`).

Checked: the relative links point at existing files; doldecomp/melee, TM-CE, brew.sh, the fork's `/releases/latest` and `/issues` answer 200 (dolphin-emu.org 403s a script but is the right URL). Docs only, so `dev.py check` wasn't run.

Not verified: following the steps end to end as a player (needs a published zip; the scripts' behaviour and messages are as ticket 02 verified them, and the README quotes their messages), the Dolphin menu names in the RVZ conversion steps against a current Dolphin, and the banner line until ticket 04.
