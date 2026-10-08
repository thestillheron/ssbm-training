# 04: Random pool of all four options

**What to build:** A pool containing all four tech options makes the opponent pick uniformly at random each rep, so the player has to react rather than guess a pattern. A scenario runs the full pool for about 40 reps and the rep log shows every option chosen and carried out, each with `forced_ok`.

**Blocked by:** 02 (Tech in and tech away), 03 (Missed tech)

**Status:** resolved

- [x] A scenario with the full pool over ~40 reps logs every one of the four options at least once (a loose check, not a distribution test)
- [x] Every rep in that run has `forced_ok`, with the player on both sides
- [ ] Each option looks and sounds as it does against a human, mixed in one run (not verified: no shots or audio checked in a mixed run)
- [x] Reps still loop and reset correctly when options change rep to rep
- [x] `dev.py check` passes
