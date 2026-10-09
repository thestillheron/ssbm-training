# 05: On-screen values and Revert

**What to build:** While editing, the current values (landing x, heights, player offset) are drawn as text using the shared menu/text component from drill-select. A Revert entry restores the compiled drill's original layout, discarding the session edits.

**Blocked by:** 01, 02, 03

**Status:** resolved

- [x] Landing x, heights and player offset are shown as text and update live
- [x] Revert restores the compiled drill's layout and the markers update
- [x] Shot of the editor showing the values text

## Answer

`src/training/layout_editor.c`: three value lines under the editor's help (`text.h`), refreshed every frame and rewritten only when they change: "Landing x 40.0", "Heights 40.0 [70.0]" (selected height in brackets; "Height range" for a range) and "Player start 20.0" (`start_dx`), one decimal so fine moves show. Revert is the editor's A button (shown in the help as "A:revert"; the editor has no menu of its own, and A was the free button): `training_drill_revert_layout()` in `drill.c` copies the compiled entry's landing point, heights and start back into the session copy, then the markers are re-placed.

Scenario `layout-edit-revert` (documented in `docs/developer-workflow.md`): `drill_start` lines read `heights=40 landing_x=0 start_dx=0`, then `heights=40,70 landing_x=40 start_dx=20`, then the compiled values after A; shots `values-edited` and `reverted` show the values text and the markers moving back.
