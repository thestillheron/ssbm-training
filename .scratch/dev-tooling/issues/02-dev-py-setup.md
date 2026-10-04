# dev.py skeleton and `setup` command

Status: ready-for-agent

Spec: `../spec.md`

## What

Create `dev.py` at the repo root. It runs on Python 3.9+ using only the standard library, because it has to run before `.venv` exists. Implement `setup <path-to-disc-image>`:

1. Create `.venv`, then install `ninja` and the repo's Python requirements (`reqs/`) into it. The other commands then use `.venv`'s ninja and Python.
2. Detect Wine (macOS only; Wine Crossover via Homebrew, needs Rosetta) and Dolphin (macOS `/Applications/Dolphin.app`; the usual Windows install locations). If either is missing, print the exact install command or link and continue. Missing Wine fails `setup` only on macOS.
3. Extract the **full** disc into `orig/GALE01/` (both `sys/` and `files/`). Prefer the decomp-toolkit (`dtk`) binary that `configure.py` already downloads. **First verify that it reads RVZ.** If it can't, fall back to a clear guided manual step using Dolphin's GUI extract.
4. Verify `orig/GALE01/sys/main.dol` against `config/GALE01/build.sha1`. On a mismatch, fail with a message about region and version: it must be NTSC-U 1.02, `GALE01`.
5. Write the detected and overridden paths to a gitignored local config file, and add that file to `.gitignore`.

If no path is given, look for a single disc image (`.rvz`/`.iso`) in the gitignored `.game/` directory at the repo root.

`setup` must be safe to re-run.

## Acceptance

- On macOS, with Jamie's RVZ, `python dev.py setup <rvz>` ends with a verified `orig/GALE01/` and a working `.venv`.
- Re-running it is a fast no-op.
- No machine-specific paths are committed.
