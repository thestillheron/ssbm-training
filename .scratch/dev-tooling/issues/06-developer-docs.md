# Developer workflow doc

Status: ready-for-agent

Blocked by: 02, 03, 04, 05

Spec: `../spec.md`

## What

- Write `docs/training-dev.md` for someone new to decomp. Cover:
  - prerequisites per OS
  - getting a disc image and why it's not in the repo
  - `setup` / `build` / `run` / `check`
  - matching vs training build, linking to the glossary
  - where training code and hooks go
  - "run `check` before merging to `master`"
  - troubleshooting: Wine quarantine on macOS, wrong disc SHA-1, Dolphin not found
- Add a short pointer to it in `CLAUDE.md`.
- Don't edit `.github/README.md`.

## Acceptance

A reader who has never seen the repo can follow the doc from clone to the "- T" title screen.
