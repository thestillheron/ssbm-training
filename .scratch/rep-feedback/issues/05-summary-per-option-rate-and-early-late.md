# 05: Summary: success rate and early/late counts per option

**What to build:** The drill summary, shown when a drill ends, gets a per-option breakdown so the player knows what to practise. Each tech option that came up shows its success rate, and how many of its reps were early and how many were late (counts from tickets 03 and 04). Options that never came up are left out. Keep it on screen with the existing summary lines (name, score, pass mark, pass or fail), using the shared text helper and the same style.

**Blocked by:** 03 (early), 04 (late).

**Status:** ready-for-agent

- [ ] The summary shows, for each tech option that came up, its success rate and its early and late counts
- [ ] Options that never came up are not listed
- [ ] The counts agree with the rep log's `timing` fields for the drill (checked against the log of a full drill run)
- [ ] A shot of the summary after a drill with at least one success, early and late, or as close as piloting allows
- [ ] `dev.py check` passes
