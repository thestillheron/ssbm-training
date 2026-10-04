# dev.py `build` and `check` commands

Status: ready-for-agent

Blocked by: 01, 02

Spec: `../spec.md`

## What

- `build`: runs the training build by default (`configure.py --training --build-dir build/training`). `build --matching` runs the matching build (upstream's default `build/GALE01/`).
- Track which build is currently configured, e.g. in a small gitignored state file. Re-run `configure.py` only when switching builds or when `configure.py` itself changed.
- On macOS, pass the detected Wine via `configure.py --wrapper` unless `configure.py` already finds it.
- `check`: builds the matching build and fails unless `main.dol` is byte-identical. Then builds the training build and fails unless it compiles and links. Print a clear pass/fail summary for each build.

## Acceptance

- Alternating `build` and `build --matching` doesn't recompile untouched objects; the second build of each kind is near-instant.
- `check` passes on a clean tree.
- `check` fails if a training hook leaks into the matching build. Test this by temporarily removing an `#ifdef MELEE_TRAINING` guard.
