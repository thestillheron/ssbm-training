# 01: Edit layout shell: move the landing point and resume

**What to build:** Edit layout in the pause menu opens the layout editor over the frozen match. The opponent hangs at the selected drop height above the landing point and the player stands at its start. The stick moves the landing x, snapped to the floor below and clamped to the last valid position. B or Start leaves the editor and resumes the drill with the edited session working copy; the compiled drill is unchanged. Fed opponent input is neutral while editing.

**Blocked by:** None (can start immediately)

**Status:** resolved

- [x] Edit layout appears in the pause menu and opens the editor with the fighters placed as markers
- [x] Stick moves the landing x, snapped to the floor, clamped at edges
- [x] Leaving resumes the drill using the edited layout; the compiled drill is not modified
- [x] Scenario: open Edit layout, move the landing point, leave; rep log drill_start / drop lines show the new layout

## Answer

Scenario `tools/scenarios/layout-edit-move` (documented in `docs/developer-workflow.md`): `drill_start` and `layout` lines read `landing_x=60`, then `86` (clamped right) and `-74` (clamped left, limited by the player's start), and the shots show the markers and the editor text.

Design, for the later tickets:

- **Session copy** lives in `src/training/drill.c`: a static `Drill session` copied from the table. `training_drill_current()` returns it (const) and `training_drill_session()` returns it writable. It is re-copied only when a different drill index is selected, so edits survive Restart, retries and re-entering the same drill; the compiled `drills[]` table is never written. Revert (ticket 05) can copy `training_drill_at(training_drill_current_index())` back into it.
- **Editor** is `src/training/layout_editor.c/.h`: `training_layout_editor_open()`, `training_layout_editor_update(port)` (false once B/Start closed it), `training_layout_editor_init()` (at match start, from `training_pause_init`). It owns the editor text and `selected_height` (index into `heights`, ticket 03). `move_landing(dx)` moves `landing_x`, places the markers, undoes the move if the layout no longer resolves (clamp), and keeps `landing_y` on the surface found. Movement: `MOVE_SPEED` 1 unit/frame at full tilt, dead zone 0.2 (ticket 02 adds fine/coarse).
- **Edit mode** in the rep loop (`src/training/tech_chase.c`): `training_tech_chase_edit_begin()` sets `editing` (opponent's fed input stays neutral in `training_cpu_input`, a pending tech press is dropped, rep result and vanish cleared); `training_tech_chase_edit_place(height_index, Layout* out)` resolves the session drill's layout with the fighters' measured reach and teleports both fighters as markers (opponent at landing + height, player grounded at start); `training_tech_chase_edit_end()` clears `editing` and restarts the drill from rep 1 (a new `drill_start` with the edited values; the markers displaced the rep in progress).
- **Pause menu** (`src/training/pause.c`): "Edit layout" is the last item (so existing scenarios' item positions are unchanged). While `editing` the pause think delegates to the editor; on close it sets `resume_pending`, which now waits for both A and B to be released before unfreezing.
