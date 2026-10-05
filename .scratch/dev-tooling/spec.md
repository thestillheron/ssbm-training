# Dev tooling: build and play-test on macOS and Windows

Status: resolved

## Problem Statement

I've forked the completed Melee decompilation to add training features, starting with tech chasing. I'm new to decomp, to this project, and to its tooling. The existing setup assumes contributors already know the upstream workflow: extracting the disc by hand, running `configure.py` and `ninja`, installing Wine on macOS, and copying a built `main.dol` into a game folder before booting it in Dolphin. There's no quick, repeatable way to go from a code change to playing the result. There's also no guard that tells me whether my changes have disturbed the original game.

I work on an Apple Silicon MacBook and on a Windows machine. I need the same workflow on both. My copy of the game is an RVZ disc image.

## Solution

A single developer entry point at the repo root, `dev.py`, with the same four commands on both machines:

- **`setup`**: prepares a machine from a fresh clone. It installs what it can, checks what it can't install, extracts my disc image, and verifies it's the right game version.
- **`build`**: builds the **training build** by default, or the **matching build** on request.
- **`run`**: builds the training build and boots it in my normal Dolphin install.
- **`check`**: proves the matching build is still byte-identical to the original game, and that the training build compiles and links.

The training build shows a permanent "- T" marker next to the "Melee" subtitle on the title screen, so I can always tell which build Dolphin has booted. A developer doc explains the workflow to someone new to decomp.

## User Stories

1. As a developer on a fresh clone, I want one `setup` command, so that I don't have to follow a multi-page manual to get a working environment.
2. As a developer, I want `setup` to accept my RVZ disc image, so that I don't have to convert it to ISO first.
3. As a developer, I want `setup` to also accept an ISO, so that a different copy of the game works too.
4. As a developer, I want `setup` to find a single disc image in the repo's gitignored game folder when I don't pass a path, so that re-running it needs no arguments.
5. As a developer, I want `setup` to extract the full disc, not just `main.dol`, so that the training build is playable, not only buildable.
6. As a developer, I want `setup` to verify `main.dol` against the expected SHA-1, so that a wrong region or version fails immediately, not as a confusing build mismatch later.
7. As a developer with the wrong disc, I want the failure message to say the game must be NTSC-U 1.02 (`GALE01`), so that I know what to fix.
8. As a developer, I want `setup` to install ninja and the repo's Python packages into a project virtual environment, so that nothing is installed globally and both machines get the same versions.
9. As a macOS developer, I want `setup` to check for Wine and print the exact install command if it's missing, so that I'm not left guessing how to run the Windows-only compiler.
10. As a macOS developer, I want `setup` to tell me about the Wine quarantine fix after OS upgrades, so that a known macOS issue doesn't block me.
11. As a developer, I want `setup` to find my Dolphin install automatically, so that I don't have to configure it in the common case.
12. As a developer, I want `setup` to tell me how to install Dolphin if it can't find it, so that I can fix it myself.
13. As a developer with non-standard install locations, I want to override the disc image, Dolphin and Wine paths in a per-machine config file, so that the tooling works on any machine without code changes.
14. As a developer, I want the per-machine config to be gitignored, so that machine-specific paths are never committed.
15. As a developer, I want `setup` to be safe to re-run, and fast when nothing has changed, so that I can run it whenever I'm unsure of my environment's state.
16. As a developer, I want the commands to be identical on macOS and Windows (apart from `python` vs `py`), so that I don't have to remember two workflows.
17. As a developer, I want `build` to produce the training build by default, so that the command I run most often is the shortest.
18. As a developer, I want `build --matching` to produce the matching build, so that I can work on or inspect the unmodified game.
19. As a developer, I want switching between the matching and training builds to avoid recompiling untouched code, so that switching is cheap.
20. As a developer, I want the matching build to stay in upstream's default output location, so that upstream tools like objdiff and upstream's docs keep working unchanged.
21. As a developer, I want `run` to build the training build and boot it in Dolphin with one command, so that the edit → play loop is as short as possible.
22. As a developer, I want `run` to use my normal Dolphin install and settings, so that my controller and graphics configuration just work.
23. As a developer, I want `run` to never modify my extracted original game files, so that the matching build's reference stays pristine.
24. As a developer, I want `run` not to duplicate the whole game's data every time, so that it's fast and doesn't fill my disk.
25. As a developer, I want `run` to pick up my latest code change every time, so that I'm never play-testing a stale build.
26. As a developer, I want "- T" next to the "Melee" subtitle on the title screen of the training build, so that I can see at a glance that I've booted my build, not the original.
27. As a developer, I want the matching build never to show the "- T" marker, so that it remains byte-identical to the original game.
28. As a developer, I want `check` to fail if the matching build isn't byte-identical, so that I know when a change has leaked into the original game.
29. As a developer, I want `check` to fail if the training build doesn't compile or link, so that broken feature code doesn't reach `master`.
30. As a developer, I want `check` to print a clear pass/fail summary for each build, so that I can see what failed without reading a wall of compiler output.
31. As a developer, I want `check` to catch a training hook that isn't properly guarded out of the matching build, so that hooks in upstream files can't silently break matching.
32. As a developer, I want training feature code to live in its own source area, so that merges from upstream `doldecomp/melee` rarely conflict.
33. As a developer, I want hooks into upstream game code to be single, clearly guarded lines, so that they're easy to find and easy to re-apply after an upstream merge.
34. As a developer, I want the `configure.py` change for the training build to be minimal, so that upstream changes to `configure.py` merge cleanly.
35. As a developer new to decomp, I want a developer doc that goes from clone to the "- T" title screen, so that I can get going without understanding upstream's whole toolchain first.
36. As a developer, I want the doc to explain matching vs training builds in the glossary's terms, so that the vocabulary stays consistent across docs, commands and issues.
37. As a developer, I want the doc to explain why the disc image isn't in the repo and what version is needed, so that I know what to supply.
38. As a developer, I want the doc to cover common failures (Wine quarantine, wrong disc SHA-1, Dolphin not found), so that I can fix them without asking.
39. As a coding agent working in this repo, I want `CLAUDE.md` to point to the developer doc, so that I use the same workflow and vocabulary as the human developer.
40. As a developer, I want "run `check` before merging to `master`" written down, so that the local gate replaces the CI the fork doesn't have.
41. As a developer on Windows, I want the whole workflow to work without WSL, msys2 or admin rights, so that I can use the machine as it is.
42. As a developer on Apple Silicon, I want the workflow to work under Rosetta-backed Wine, so that my MacBook is a first-class development machine.
43. As a maintainer of this tooling, I want an automated test suite that drives the real commands, so that I can change `dev.py` internals without fear.
44. As a maintainer, I want tests that need the game's data to skip with a clear message when the disc image is absent, so that the suite still runs cleanly on a machine without it.

## Implementation Decisions

- **Two builds, both kept** (ADR-0001). The matching build stays byte-identical on every commit and is the regression check. The training build is the game code plus training features, compiled non-matching. Terms are as defined in the glossary.
- **Upstream sync.** We keep pulling `doldecomp/melee`. Training code lives in its own source area. Upstream files are touched only by small hooks, each guarded by a compile-time training switch that the matching build compiles out completely.
- **Training build switch in `configure.py`.** A new `--training` option implies non-matching, adds the training library, and defines the training switch for all compiled units. The diff to `configure.py` is kept as small as possible.
- **Separate output locations.** The matching build uses upstream's default build directory. The training build uses its own directory under `build/`. Because `configure.py` writes a single `build.ninja` at the repo root, `dev.py` records which build is currently configured and re-runs `configure.py` only when switching builds, or when `configure.py` itself has changed.
- **Native toolchain behind one entry point** (ADR-0002). Each OS uses its native toolchain: Python and ninja everywhere, plus Wine on macOS to run the MWCC compiler. Nix and Docker were rejected. `dev.py` sits at the repo root, a new file upstream won't touch. It uses only the Python standard library, so it runs before the virtual environment exists.
- **Dependencies.** `setup` installs ninja and the repo's Python requirements into a project virtual environment. Wine (macOS only) and Dolphin are detected, not installed. If either is missing, `setup` prints the exact install command or link. A missing Wine fails `setup` only on macOS.
- **Game data.** The user supplies their own disc image, either RVZ or ISO. `setup` extracts the full disc into the repo's gitignored original-files area and verifies `main.dol` against the repo's recorded SHA-1. Extraction should use the decomp-toolkit binary the build already downloads, which is expected to read RVZ. Confirm this first. If it can't, fall back to a guided manual extraction step through Dolphin's GUI. Don't rely on DolphinTool, because it isn't bundled with Dolphin on macOS. If no path is given, `setup` looks for a single disc image in the repo's gitignored `orig/GALE01/` folder.
- **Machine-specific paths.** The disc image, Dolphin and Wine paths are auto-detected first. A gitignored per-machine config file holds anything that isn't found and any override. Known defaults include Dolphin at its standard macOS application location and the usual Windows install locations.
- **Play-test output.** `run` assembles a playable game folder in the training build's output directory. It mirrors the extracted original disc using links (hard links on Windows, which need no admin rights on the same volume), falls back to copying with a warning, and replaces only `main.dol`. The original extracted files are never written to.
- **Emulator.** Vanilla Dolphin only, using the user's normal install and settings. `run` launches it booting the assembled game's `main.dol` directly, and doesn't block waiting for Dolphin to exit. There's no Slippi compatibility goal.
- **Training-build marker.** The "Melee" subtitle is part of the 3D logo model in the title screen's data archive, not text, so we don't edit it. Instead, the training build draws "- T" in the game font next to it, using the same text-drawing API the title screen already uses to show the debug build timestamp. The drawing code lives in the training source area. The title screen gets one guarded hook call. Position and scale are tuned by eye. The marker is permanent.
- **`check` semantics.** Run the matching build and fail unless the build's existing SHA-1 verification passes. Then run the training build and fail unless it compiles and links. Print a per-build pass/fail summary, and exit non-zero on any failure.
- **No CI on the fork.** `main.dol` can't legally be committed or stored as a secret, and upstream's workflow only runs on the upstream repo. `dev.py check` run locally is the gate before merging to `master`.
- **Docs.** A new developer workflow doc in the docs area, plus a pointer to it in `CLAUDE.md`. Upstream's README is left untouched to avoid merge conflicts.

## Testing Decisions

- **What makes a good test here.** Tests drive the real `dev.py` commands as a user would, and assert only on what's visible from outside: exit codes, printed pass/fail summaries, and files on disk. They don't import or call `dev.py`'s internal functions, so internals can be restructured freely.
- **One seam: the `dev.py` command line.** Covers:
  - `setup`: extracts the disc, passes SHA-1 verification, rejects a wrong or corrupt image with the version message, and is a fast no-op when re-run.
  - `build` / `build --matching`: produce `main.dol` in their respective output locations. Switching back and forth doesn't recompile untouched objects.
  - `check`: passes on a clean tree. It fails when a training hook is deliberately left unguarded so it leaks into the matching build. This is the key regression test.
  - `run`: the Dolphin path is overridden, through the existing per-machine config, with a small fake executable that records its arguments. The tests assert that it was launched with the assembled game's `main.dol`, that the assembled folder contains the training build's `main.dol`, and that the original extracted files' SHA-1 is unchanged.
- **Existing seam reused.** The matching build's SHA-1 verification against the recorded hash is the source of truth for "byte-identical". `check` relies on it rather than reimplementing it.
- **Framework.** Python's built-in `unittest`, so the tests need nothing installed and can run before the virtual environment exists. The repo has no Python unit-test prior art to follow; the `tools/check` "tests" are source-style checks, not unit tests.
- **Game data dependency.** Tests that need the disc image skip with a clear message when it's absent. The suite is run locally on the developer's machines, which is consistent with having no CI.
- **Manual verification.**
  - The "- T" marker appearing in Dolphin.
  - The full `setup` → `run` → `check` flow on the Windows machine.

  Neither is practical to automate until in-emulator testing exists.

## Out of Scope

- Automated in-emulator tests: scripted controller inputs and assertions on game memory. This is deferred to a later effort, which will likely bring a project-local Dolphin profile with fixed settings.
- A project-local or portable Dolphin configuration.
- Packaging the training build as an ISO or RVZ for sharing.
- CI on the fork.
- Slippi Dolphin compatibility.
- The experimental CMake/aurora native PC build.
- Any actual training features beyond the "- T" marker.

## Further Notes

- **Done when:** on both the Mac and the Windows machine, a fresh clone runs `setup` with the RVZ, then `run`, and lands on Melee's title screen showing "- T". `check` must also pass on both. The Windows verification is a human step.
- **Related records:** ADR-0001 (keep the matching build alongside the training build) and ADR-0002 (native toolchain behind one Python entry point). The glossary entries are **matching build** and **training build**.
- **Disc image location:** the gitignored `orig/GALE01/` folder (upstream's designated spot; the developer's ISO is `orig/GALE01/Melee.iso`). No separate `.game` folder.
