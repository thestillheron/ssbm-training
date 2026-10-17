# ISO release: players build their own RTC Training disc

Status: ready-for-agent

## Problem Statement

A player who wants to train reaction tech chasing can't get the training build at all unless they're a developer. The only way in is to clone the repo, install the toolchain, extract their disc and run `dev.py run`, which boots a loose game folder in Dolphin, not a disc. Nothing can be downloaded, no ISO shows up in Dolphin's game list, and the repo's front page is still upstream decomp's README, which doesn't mention training. The repo is about to go public, and players need a way to turn their own copy of Melee into [RTC Training](../../docs/glossary.md#glossary_rtc_training) without being given the game.

## Solution

We follow [Training Mode - Community Edition](https://github.com/AlexanderHarrison/TrainingMode-CommunityEdition)'s model ([ADR-0008](../../docs/adr/0008-players-get-a-patch-not-the-game.md)). Each GitHub release on the fork has a zip with a [release patch](../../docs/glossary.md#glossary_release_patch). A player drags their vanilla Melee 1.02 ISO onto a script in the zip (Windows) or runs a shell script (macOS, Linux), and gets an RTC Training ISO. Dolphin lists it apart from Melee, with its own banner, name and save. Developers build the same ISO from source with one `dev.py` command and make the release zip with another. The repo's README becomes a page for players: what RTC Training is, how to get it and how to report bugs, with building from source as a short section that links to the developer docs.

## User Stories

1. As a player, I want to download RTC Training from the repo's releases page, so that I can train without cloning or building anything.
2. As a player, I want the download to need only my own vanilla Melee ISO, so that I never have to find the game somewhere dubious.
3. As a Windows player, I want to drag my ISO onto a script and get the RTC Training ISO, so that I don't need a terminal.
4. As a macOS or Linux player, I want a script I run with my ISO's path, so that I can make the ISO on my OS too.
5. As a macOS or Linux player, I want the script to tell me to install xdelta if I don't have it, so that I know what's missing.
6. As a player with the wrong disc (another region, revision, a modified ISO, an RVZ or NKit), I want a clear "not a valid v1.02 NTSC melee iso" error, so that I know my source is the problem and not the download.
7. As a player with an RVZ, I want the README to tell me to convert it to ISO in Dolphin first, so that I can use the copy I already have.
8. As a player, I want the patched ISO written next to the scripts with a predictable name, so that I can find it and add it to Dolphin.
9. As a player, I want my original ISO left untouched, so that I can still play vanilla Melee.
10. As a player, I want RTC Training listed in Dolphin with its own name, banner and game ID, so that I can tell it apart from my vanilla Melee.
11. As a player, I want RTC Training to keep its own save, so that it can never overwrite my vanilla Melee save.
12. As a player, I want the save to be called RTC Training in Dolphin's memory card manager, so that I know which file is which.
13. As a player, I want release builds to leave out the [authoring tools](../../docs/glossary.md#glossary_authoring_tools), so that I only see features meant for players.
14. As a player, I want the README to say what RTC Training contains (RTC 101, the custom drill and drill history), so that I know what I'm getting before I download.
15. As a player, I want the README to say it is tested on Dolphin, so that I know what to expect on console.
16. As a player, I want to report bugs through GitHub Issues, so that I have somewhere to send problems.
17. As a player, I want to see the credits, including TM-CE and UnclePunch, so that I know whose work this builds on.
18. As a player, I want release versions numbered (`v0.1.0` onward), so that I can tell whether I have the latest.
19. As a developer, I want one `dev.py` command that builds the RTC Training ISO from source, so that I can boot exactly what players get.
20. As a developer, I want that command to download the ISO packing tool the first time and reuse it afterwards, as `setup` does with dtk, so that I don't have to install it by hand.
21. As a developer on macOS (Apple Silicon), I want the ISO command to work as it does on Windows, so that both machines produce the same ISO (ADR-0002).
22. As a developer, I want `build` and `run` to stay as fast as they are now, so that writing a 1.4 GB ISO only happens when I ask for it.
23. As a developer, I want the ISO build always to leave out the authoring tools and use the default tech pool, whatever I last configured, so that the ISO is the same as the release.
24. As a developer, I want one `dev.py` command that makes the release zip, so that publishing is repeatable and doesn't depend on my memory.
25. As a developer whose disc is an RVZ, I want the release command to make a plain ISO from it before encoding, so that the patch applies to the ISO players have.
26. As a developer, I want the release command to refuse a source disc that fails verification, so that I never publish a patch that only applies to my own bad dump.
27. As a developer, I want the release command to say how to install xdelta on macOS when it's missing, so that I can fix it straight away.
28. As a developer, I want files under the extracted disc never written by the ISO or release commands, so that my source disc stays intact.
29. As a developer, I want `dev.py check` to encode the banner along with the archive patches, so that a bad banner image fails before a run or release.
30. As a developer, I want `run` to show the same banner and title as the ISO, so that what I test matches what ships.
31. As a developer, I want the developer workflow doc to cover the ISO and release commands and the manual upload, so that anyone can make a release.
32. As a developer, I want upstream syncs to keep our README when it conflicts, so that merging decomp progress doesn't overwrite the player page.
33. As an agent, I want my work to stay in the local `.scratch/` tracker after Issues are on, so that I never file to upstream through `gh`'s default repo.
34. As the maintainer, I want to design the banner art myself, so that the game list shows something deliberate.
35. As the maintainer, I want to make the repo public, turn on Issues and upload the first release by hand, so that I decide when it goes out.

## Implementation Decisions

- **Name.** "RTC Training" is the disc header title, README heading, banner name and save comment. `rtc-training` is the file stem for the ISO, the zip and the zip's folder.
- **Source disc.** Only `GALE01` revision 1.02, as a plain ISO. RVZ has to be converted first (Dolphin does this losslessly). NKit, JP and PAL discs aren't supported.
- **ISO packing.** We use gc_fst (Apache-2.0, by TM-CE's maintainer). Its rebuild packs the game folder `assemble_game` produces, and its set-header writes `GTCE01` (ADR-0006) and the title. The existing tool downloader fetches it like dtk: on Windows and Linux from gc_fst's releases, and on macOS from the arm64 binary TM-CE keeps in its repo, at a pinned commit, because gc_fst publishes no macOS release. Not yet checked: that rebuild accepts our `sys/` + `files/` layout. That's the first thing ticket 01 checks.
- **`dev.py iso`.** It configures the training build without authoring tools and with the default tech pool, builds it, runs `assemble_game` and packs the folder into the ISO in the training build's output directory. `build` and `run` stay as they are. `iso` needs neither Dolphin nor window focus.
- **`dev.py release`.** It runs the ISO build and makes a plain ISO of the developer's vanilla disc (dtk convert if the disc isn't already one). It refuses to continue unless dtk verify passes, then encodes the patch with xdelta3 and zips it. The zip holds one folder containing the patch, `xdelta3.exe`, a Windows drag-and-drop `.bat` and a macOS/Linux `.sh`, both modelled on TM-CE's. Each script writes the RTC Training ISO beside itself and reports a wrong source disc. On the developer's machine, xdelta3 is downloaded on Windows. On macOS, `setup` checks for it and prints the brew command when it's missing.
- **Banner.** The disc's `opening.bnr` (a 96×32 RGB5A3 image, plus short name, maker and description) becomes a patched copy in the assembled game folder, made the same way as the main menu archives (ADR-0007). So `run`, `iso` and `check` all go through it, and the extracted disc is never written. The maintainer designs the art and text with Claude Design and commits them as assets.
- **Save comment.** The comment on the training build's save becomes "RTC Training". It used to read "Melee Tech Chase Training".
- **README.** We replace the README that GitHub shows on the repo's front page (the copy under `.github/`, which takes precedence over a root README) with a player page modelled on TM-CE's. It has the banner, a one-line summary, a download link, steps to apply the patch on each OS, RVZ conversion, contents, the separate-save note, "tested on Dolphin", bug reports through Issues, a short "Building from source" section, a link to upstream doldecomp/melee, and the credits.
- **Upstream syncs.** On conflict, the player README wins. The developer workflow doc records this.
- **Issues.** GitHub Issues are turned on for players' bug reports. Agent work stays in `.scratch/`, and the agent issue-tracker doc still forbids filing to GitHub.
- **Releases.** There's no CI, because the matching `main.dol` can't be stored publicly (ADR-0001). The maintainer runs the release command and uploads the zip by hand under a `vX.Y.Z` tag, starting at `v0.1.0`.
- **Credits.** The credits file lists Alex Harrison (Aitch), TM-CE's maintainer, for the release model and gc_fst. Already done.

## Testing Decisions

- **One seam: the real `dev.py` commands.** Tests drive `iso`, `release` and `check` the way the existing tooling tests drive `build`, `run` and `check` (`test_run`, `test_check`, `test_build`), and they skip when no disc image is present. They check external results only: files written, headers, bytes and exit codes. They don't call internal functions. The banner encoder gets no seam of its own. It is tested through `check`'s patch step and the assembled game folder.
- **`iso`.** The output exists. dtk's disc info reports game ID `GTCE01` and title "RTC Training". The `opening.bnr` inside it is ours, not vanilla's. Nothing under the extracted disc changes.
- **`release`.** The zip has the expected folder and files. Applying its patch to a copy of the vanilla ISO with xdelta3 gives an ISO byte-identical to `iso`'s. Applying it to anything else fails. The build in the zip never has authoring tools, even if the last configured build had them.
- **`check`.** A banner asset that can't be encoded (wrong size, for example) makes the archive-patches line FAIL.
- **Manual, recorded in the tickets.** The ISO boots in Dolphin to the main menu with "- T" and shows the banner and name in the game list. Ask the user before launching Dolphin, and run `dev.py stop` afterwards. The `.bat` works by drag and drop on Windows and the `.sh` works on macOS. A new save shows "RTC Training" in the memory card manager.
- `dev.py check` passes, and the matching build is unchanged.

## Out of Scope

- CI builds or automated release uploads.
- JP, PAL or NKit source discs.
- Claims of console (Nintendont, Swiss) support.
- A Discord or other community channel.
- Any change to the matching build.

## Further Notes

- Tickets live in `issues/`. 04 (banner art) and 06 (go public and publish `v0.1.0`) are the maintainer's. 01–03 and 05 are for agents.
- The release patch is only as good as the developer's source ISO. That's why `release` makes verification a hard gate and doesn't just warn.
