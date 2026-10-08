# 07: Stage match and temporary drill chooser

**What to build:** A temporary way to choose the current drill until drill-select exists: a debug input in authoring builds switches between table entries. A curriculum drill runs only on its named stage, so the chooser offers only drills for the stage in play and says which stage the others need.

**Blocked by:** 01: Drill record and table, rep loop reads the current drill. Best done after 02 so more than one drill exists to switch between.

**Status:** ready-for-agent

- [ ] The debug input switches the current drill in authoring builds only
- [ ] Only drills for the stage in play can be selected
- [ ] Drills for other stages are listed with the stage they need
- [ ] Switching the drill logs a new `drill_start` line
- [ ] `dev.py check` passes
