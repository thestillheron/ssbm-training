# Add the training build to configure.py

Status: ready-for-agent

Spec: `../spec.md`

## What

- Add a `configure.py --training` flag. It implies `--non-matching`.
- When the flag is set, add a training library whose sources live under `src/training/`, with objects marked `Equivalent`. Also define `MELEE_TRAINING` for every compiled unit, including upstream units, so hooks in upstream files can be guarded.
- Keep the diff to `configure.py` as small as possible, so upstream merges stay easy (ADR-0001).
- Add a placeholder source under `src/training/` so the library isn't empty. Ticket 05 replaces it with the real marker.

## Acceptance

- `python configure.py && ninja` (the matching build) still reports a byte-identical `main.dol`.
- `python configure.py --training --build-dir build/training && ninja` builds and links a `main.dol`. Its output path is under `build/training/`.
