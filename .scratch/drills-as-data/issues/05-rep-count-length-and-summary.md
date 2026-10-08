# 05: Rep-count length, end of drill and summary

**What to build:** A drill with a rep-count length ends after that many counted reps; void reps don't count. When the length is reached and the current rep finishes, dropping stops and a text summary is drawn showing the score, the pass mark, pass or fail, and the successes per tech option. From the summary the player can restart the drill (restart is the only action until drill-select exists). The rep log gets a `drill_end` line with the score and whether it passed.

**Blocked by:** 01: Drill record and table, rep loop reads the current drill.

**Status:** resolved

- [x] A rep-count drill ends after exactly N counted reps, with voids excluded
- [x] The summary shows score, pass mark, pass/fail and per-option successes (verified by a screenshot)
- [x] The `drill_end` log line has the score and passed, and a scripted no-input run gives 0 successes and a fail
- [x] Restarting from the summary starts the drill again from zero
- [x] `dev.py check` passes


## Answer

A rep-count drill now ends after `length` counted reps (`drill_length_reached` in `src/training/tech_chase.c`; voids and abandoned reps are never recorded, so they are not counted). After the last rep's pause the loop enters a Summary state: dropping stops, `drill_end rep=N frame=F score=S/C pass_mark=P passed=true|false` is logged, and `training_summary_show` (`src/training/score.c`) draws the name, score, pass mark, PASS/FAIL and per-option successes (per-option counts are kept in the score). Pressing A on the player's pad (`HSD_PadGameStatus` trigger) hides the summary, zeroes the score, resets the rep number, logs a new `drill_start` and starts again. Verified with scenario `drill-end-no-input` (`score=0/10 pass_mark=7 passed=false`, summary and after-restart shots); `dev.py check` passes.
