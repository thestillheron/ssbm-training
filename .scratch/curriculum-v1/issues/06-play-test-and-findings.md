# 06: Play-test the curriculum across matchups

**What to build:** A play-test pass over the whole [curriculum](../../../docs/glossary.md#glossary_curriculum), by the developer, across several matchups including extremes such as Bowser vs Sheik tech away. It checks that each drill's pass mark means readiness for the next, and records the findings in this file. Two outputs matter beyond the drills themselves:

- The real length of a rep, measured from the rep log. The rep counts assume ~200 frames; if the measured length is more than ~15% off, rescale the counts so one-option drills take about 2 minutes and the rest about 4.
- Problem matchups, written up as candidates for the later per-matchup overrides item (see the Later list in `.scratch/priorities.md`).

**Blocked by:** 05 (all curriculum drills in place).

**Status:** resolved

- [ ] Every curriculum drill has been played in at least one matchup, and the pass marks have been adjusted where a drill was clearly too hard or too easy
- [ ] The measured rep length is recorded here, and the rep counts are rescaled if it is more than ~15% off
- [ ] Problem matchups are listed under "Findings" below, each with the drill and why the punish fails
- [ ] `dev.py check` passes after any table changes

## Findings

To be filled in by the developer.
