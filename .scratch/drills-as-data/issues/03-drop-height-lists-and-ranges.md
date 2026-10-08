# 03: Drop height lists and ranges

**What to build:** A drill's drop heights can be a single value (every rep has the same timing), a list picked at random each rep, or a min–max range picked uniformly each rep. Heights are in game units above the snapped landing surface.

**Blocked by:** 01: Drill record and table, rep loop reads the current drill.

**Status:** resolved

- [x] A one-height test drill produces identical `landing` timing across reps in the rep log
- [x] A three-height list test drill produces every height across enough reps
- [x] A range test drill produces heights that all fall within the range
- [x] The height setting appears in the `drill_start` line
- [x] `dev.py check` passes

## Answer

A drill's heights are now used as data: `training_drill_pick_height()` (`src/training/drill.c`) returns the only height, a `HSD_Randi` pick from the list, or `heights[0] + HSD_Randf() * (heights[1]-heights[0])` for a range; `do_rep_setup` uses it and the `drop` line logs the picked height. `drill_start` now reads `height_kind=list|range heights=a,b,c`. For acceptance, `dev.py build/run --drill N` (`configure.py --drill`, `TRAINING_DRILL_INDEX`) picks the current drill, and two authoring-only test drills were added to the table (index 1: list 30/60/90; index 2: range 30..90). New scenario `drill-heights` (60 s no input). Verified in Dolphin: default drill gives height=40 and landing 31 frames after the drop every rep; `--drill 1` produced 30, 60 and 90; `--drill 2` produced 30..85, all within range. `dev.py check` passes.
