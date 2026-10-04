# Dev tooling: build and play-test on macOS and Windows

## Goal

One set of commands, identical on macOS (Apple Silicon) and Windows, that takes a fresh clone to Melee booting in Dolphin from our **training build**, and checks that the **matching build** is still byte-identical. See `docs/glossary.md` for both terms.

## Decisions

- **Testing scope:** a build check plus a one-command manual play-test. Automated in-emulator tests (scripted inputs, memory assertions) are deferred to a later effort.
- **Upstream:** we keep pulling `doldecomp/melee`. Training code lives in `src/training/`, and upstream files are touched only through small `#ifdef MELEE_TRAINING` hooks. See ADR-0001.
- **Two builds:** the matching build stays at upstream's default output dir `build/GALE01/`. The training build goes to `build/training/`. See ADR-0001.
- **Toolchain:** native on each OS (Python + ninja, plus Wine on macOS for MWCC) behind a single entry point, `dev.py`, at the repo root. No Nix, no Docker. See ADR-0002.
- **Emulator:** vanilla Dolphin only, using the user's normal install and settings. No Slippi compatibility goal. A project-local Dolphin profile is deferred until automated tests need one.
- **Game data:** the user supplies their own disc image, currently an **RVZ**. `setup` extracts the full disc into `orig/GALE01/` and verifies `main.dol` against `config/GALE01/build.sha1`. `orig/` is never modified after that.
- **Play-test output:** an extracted game folder under `build/`, made of links to `orig/GALE01/` with only `main.dol` replaced, booted directly by Dolphin. ISO/RVZ packaging is deferred.
- **Machine-specific paths** (disc image, Dolphin, Wine): auto-detected first, with a gitignored local config file for anything not found or overridden.
- **Dependencies:** `setup` installs ninja and the Python packages into `.venv` automatically. For Wine and Dolphin it checks they're present and prints the exact install command if not.
- **CI:** none. `main.dol` can't be committed or stored legally, so `dev.py check` run locally is the gate before merging to `master`.
- **Docs:** `docs/training-dev.md`, plus a pointer in `CLAUDE.md`. Upstream's `.github/README.md` is not edited.
- **Training-build marker:** the title screen shows "- T" next to the "Melee" subtitle, drawn as game-font text using the same text API as the debug timestamp in `gmtitle.c`. The "Melee" subtitle itself is a texture in `GmTtAll.dat`, so we don't edit it. The marker is permanent.

## Command surface

```
python dev.py setup <path-to-disc-image>   # Windows: py dev.py ...
python dev.py build [--matching]           # training build by default
python dev.py run                          # training build -> assemble game folder -> launch Dolphin
python dev.py check                        # matching build byte-identical AND training build compiles and links
```

`dev.py` records which build is currently configured. It re-runs `configure.py` only when switching between builds, because `configure.py` writes a single `build.ninja` at the repo root.

## Done when

On **both** the Mac and the Windows machine, a fresh clone runs `setup <rvz>` → `run`, ends up on Melee's title screen showing "- T", and `check` passes.

## Out of scope (for now)

Automated in-emulator tests, a project-local Dolphin profile, ISO/RVZ packaging, CI on the fork, Slippi, and the experimental CMake/aurora native build.
