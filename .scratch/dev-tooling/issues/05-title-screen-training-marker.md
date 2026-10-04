# Title-screen "- T" marker for the training build

Status: ready-for-agent

Blocked by: 01

Spec: `../spec.md`

## What

In the training build only, draw "- T" next to the "Melee" subtitle on the title screen.

- The subtitle is part of the 3D logo in `GmTtAll.dat`, so we can't edit it. Draw game-font text using the same `HSD_SisLib` calls as the debug build timestamp in `gm_Scene_Title_OnEnter` (`src/melee/gm/gmtitle.c`).
- Put the drawing code in `src/training/`. Upstream `gmtitle.c` gets only a single call, guarded by `#ifdef MELEE_TRAINING`.
- Tune the position and scale until it sits just after "Melee". This needs a few `dev.py run` iterations.
- The marker is permanent, not a temporary test.

## Acceptance

- `dev.py run` shows "- T" beside "Melee".
- `dev.py check` still passes: the matching build is byte-identical and shows no marker.
