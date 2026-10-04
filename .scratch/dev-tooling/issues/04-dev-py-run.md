# dev.py `run` command

Status: ready-for-agent

Blocked by: 02, 03

Spec: `../spec.md`

## What

1. Run the training build, the same as `dev.py build`.
2. Assemble a playable game folder under `build/training/` that mirrors `orig/GALE01/`, with `sys/main.dol` replaced by the training build's `main.dol`. Use links so we don't copy about 1.3 GB: hard links work without admin rights on Windows if they're on the same volume. Fall back to copying, with a warning. Never write into `orig/`.
3. Launch the user's Dolphin and boot that folder's `main.dol` directly (Dolphin's `-e`/`--exec`). Use the configured Dolphin path. Don't wait on Dolphin's exit unless that turns out to be necessary.

## Acceptance

- On macOS, `python dev.py run` boots Melee in Dolphin from the training build.
- Re-running after a code change picks up the new `main.dol`.
- `orig/GALE01/` is unchanged; check with its SHA-1.
