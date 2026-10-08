# 04: Per-drill behaviour switches: pool, vanish, reset, facing

**What to build:** Each drill sets its tech option pool (any subset of the four options), whether the opponent vanishes, whether the player is reset each rep, and the opponent's facing (toward the player by default, or away, left or right). With reset off, the next rep chases from wherever the player ended up.

**Blocked by:** 01: Drill record and table, rep loop reads the current drill.

**Status:** ready-for-agent

- [ ] Test drills with different option pools only ever produce options from their pool
- [ ] Vanish on and off behave as set per drill
- [ ] With reset player off, the player is not repositioned between reps
- [ ] The opponent faces toward the player by default, and the away, left and right overrides work
- [ ] All of these settings appear in the `drill_start` line
- [ ] `dev.py check` passes
