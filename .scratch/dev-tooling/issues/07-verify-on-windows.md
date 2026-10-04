# Verify the workflow end-to-end on Windows

Status: ready-for-human

Blocked by: 06

Spec: `../spec.md`

## What

On the Windows machine, from a fresh clone, follow `docs/training-dev.md`: `py dev.py setup <rvz>` → `py dev.py run` → `py dev.py check`. File any failures as new issues in this directory.

## Acceptance

- Melee boots in Dolphin showing "- T".
- `check` passes on Windows.
