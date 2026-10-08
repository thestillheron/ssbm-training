# 03: Drop height lists and ranges

**What to build:** A drill's drop heights can be a single value (every rep has the same timing), a list picked at random each rep, or a min–max range picked uniformly each rep. Heights are in game units above the snapped landing surface.

**Blocked by:** 01: Drill record and table, rep loop reads the current drill.

**Status:** ready-for-agent

- [ ] A one-height test drill produces identical `landing` timing across reps in the rep log
- [ ] A three-height list test drill produces every height across enough reps
- [ ] A range test drill produces heights that all fall within the range
- [ ] The height setting appears in the `drill_start` line
- [ ] `dev.py check` passes
