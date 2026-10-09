# 02: Player start via C-stick, plus fine/coarse toggle

**What to build:** The C-stick moves the player start offset, snapped to the floor below, with the player marker updating live. Holding a trigger or Z switches both sticks to fine movement.

**Blocked by:** 01

**Status:** resolved

- [x] C-stick moves the player start, snapped to a valid floor
- [x] Holding the fine modifier reduces movement step for both the stick and the C-stick
- [x] Scenario: edit the player start, leave; the drill resumes with the new offset

## Answer

`src/training/layout_editor.c`: the C-stick x moves the session drill's `start_dx` through the same move-and-undo as the landing x (`move_x`), so the player marker snaps to the floor and clamps at the edges. Z, or L/R pressed past 0.3, scales both sticks by `FINE_FACTOR` (0.1). Scenario `tools/scenarios/layout-edit-player-start` (documented in `docs/developer-workflow.md`): `drill_start` lines read `start_dx=40`, then `30` with `landing_x=10` (fine), then `-83` (clamped, `start_x=-86`); the shots show the player marker moving.
