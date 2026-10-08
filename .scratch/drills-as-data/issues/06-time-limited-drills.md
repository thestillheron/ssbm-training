# 06: Time-limited drills

**What to build:** A drill can use a time limit instead of a rep count. When time runs out, the rep in progress is allowed to finish before the summary appears, so the last attempt isn't cut off.

**Blocked by:** 05: Rep-count length, end of drill and summary.

**Status:** ready-for-agent

- [ ] A time-limited test drill stops dropping new reps once time is up
- [ ] The rep in progress at the time limit finishes and counts before the summary appears
- [ ] The length kind and value appear in the `drill_start` line
- [ ] `dev.py check` passes
