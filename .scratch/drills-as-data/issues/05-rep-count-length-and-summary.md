# 05: Rep-count length, end of drill and summary

**What to build:** A drill with a rep-count length ends after that many counted reps; void reps don't count. When the length is reached and the current rep finishes, dropping stops and a text summary is drawn showing the score, the pass mark, pass or fail, and the successes per tech option. From the summary the player can restart the drill (restart is the only action until drill-select exists). The rep log gets a `drill_end` line with the score and whether it passed.

**Blocked by:** 01: Drill record and table, rep loop reads the current drill.

**Status:** ready-for-agent

- [ ] A rep-count drill ends after exactly N counted reps, with voids excluded
- [ ] The summary shows score, pass mark, pass/fail and per-option successes (verified by a screenshot)
- [ ] The `drill_end` log line has the score and passed, and a scripted no-input run gives 0 successes and a fail
- [ ] Restarting from the summary starts the drill again from zero
- [ ] `dev.py check` passes
