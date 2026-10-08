# 06: Time-limited drills

**What to build:** A drill can use a time limit instead of a rep count. When time runs out, the rep in progress is allowed to finish before the summary appears, so the last attempt isn't cut off.

**Blocked by:** 05: Rep-count length, end of drill and summary.

**Status:** resolved

- [x] A time-limited test drill stops dropping new reps once time is up
- [x] The rep in progress at the time limit finishes and counts before the summary appears
- [x] The length kind and value appear in the `drill_start` line
- [x] `dev.py check` passes

## Answer

`drill_length_reached` (`src/training/tech_chase.c`) now handles `DrillLength_Time`: a `drill_frames` clock counts think ticks outside Settle and the Summary (reset at init and on restart), and the check only runs after a rep's pause, so the rep in progress when time runs out finishes and counts first. `drill_start` logs `length=reps/N` or `length=time/S`. Added authoring test drill 10 (5 before merging with ticket 04) ("Test: time limit", 15 s, pass mark 5) and scenario `drill-time-limit` (`dev.py run --scenario drill-time-limit --drill 10`): the log showed `length=time/15`, rep 8 started at frame 894 (just before the 900-frame limit), finished with its `outcome`, then `drill_end rep=8 score=0/8 passed=false` with no later `start`; the summary shot shows the pass mark against counted reps. `dev.py check` passes.
