# 07: Stage match and temporary drill chooser

**What to build:** A temporary way to choose the current drill until drill-select exists: a debug input in authoring builds switches between table entries. A curriculum drill runs only on its named stage, so the chooser offers only drills for the stage in play and says which stage the others need.

**Blocked by:** 01: Drill record and table, rep loop reads the current drill. Best done after 02 so more than one drill exists to switch between.

**Status:** resolved

- [x] The debug input switches the current drill in authoring builds only
- [x] Only drills for the stage in play can be selected
- [x] Drills for other stages are listed with the stage they need
- [x] Switching the drill logs a new `drill_start` line
- [x] `dev.py check` passes

## Answer

D-pad right (authoring builds only, any state after the scene settles) moves the current drill to the next table entry that fits the stage in play (`Stage_80225194()`), wrapping, then restarts the drill from rep 1. The current drill is now a runtime index in `drill.c` (`training_drill_choose_next`, `training_drill_fit_to_stage`); on entering Training a drill that does not fit the stage in play is replaced by the first that does. On each switch the screen lists the new drill plus each drill needing another stage with "needs <stage>" (gone after 3 s), and the rep log gets `drill_choose`, `drill_needs` and a new `drill_start`. Acceptance: `python dev.py run --scenario drill-chooser` (shot `chooser`; the Battlefield platform drill is never started on Final Destination).
