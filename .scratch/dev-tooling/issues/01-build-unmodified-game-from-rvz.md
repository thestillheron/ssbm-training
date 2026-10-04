# 01: Build the unmodified game from the RVZ

Spec: `../spec.md`

**What to build:** On a fresh clone, a developer runs `dev.py setup <disc-image>` with their RVZ (or ISO). Setup does three things:
- It creates the project virtual environment, with ninja and the repo's Python packages installed.
- It extracts the full disc into the gitignored original-files area.
- It verifies `main.dol` against the recorded SHA-1, and rejects a wrong disc.

The developer then runs `dev.py build --matching` and gets the byte-identical **matching build**.

This ticket also establishes the test suite at the `dev.py` command-line seam, using Python's built-in `unittest`. Tests drive the real commands and assert on exit codes, output and files. Tests that need the disc image skip with a clear message when it's absent.

Extraction should use the decomp-toolkit binary the build already downloads. Confirm first that it reads RVZ. If it can't, fall back to a guided manual extraction step through Dolphin's GUI. Don't rely on DolphinTool, which isn't bundled with Dolphin on macOS.

`dev.py` uses only the Python standard library, so it runs before the virtual environment exists.

**Blocked by:** None (can start immediately)

**Status:** ready-for-agent

- [ ] On the Mac, `setup` with Jamie's RVZ extracts the full disc (system files and game files) and passes SHA-1 verification.
- [ ] `setup` with an image that isn't NTSC-U 1.02 (`GALE01`), or is corrupt, exits non-zero without leaving a half-extracted disc that looks valid.
- [ ] `build --matching` produces a `main.dol` that passes the build's existing SHA-1 check, in upstream's default output location.
- [ ] The test suite runs with no third-party dependencies. Disc-dependent tests skip cleanly when no image is present.
