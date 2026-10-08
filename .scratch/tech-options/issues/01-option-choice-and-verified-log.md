# 01: Option choice and verified log

**What to build:** Each rep chooses its tech option before the drop, uniformly at random from an option pool, using the game's own random number function. The pool and the missed-tech down time live together in one place (a drill-agnostic default for now: the pool is tech in place only, so drills-as-data can supply them later). The rep log gains `option` (the one chosen), `result_state` (what the game actually entered: tech in place, roll resolved to in or away, or knockdown) and `forced_ok` (the two agree), and a mismatch is flagged. The option is driven by a per-option input step, so adding options later is a matter of filling in that step, not reworking the rep loop. With a pool of one, every rep chooses that option; only tech in place is carried out so far.

**Blocked by:** None (can start immediately)

**Status:** resolved

- [x] The option is chosen before the drop, and the choice is readable by later features
- [x] The random choice uses the game's random number function and is uniform over the pool
- [x] The pool and the missed-tech down time are defined in one place; the default pool is tech in place only
- [x] Every rep log shows `option`, `result_state` and `forced_ok`; a rep whose forcing failed (e.g. a mistimed press) is flagged
- [x] A scenario with a pool of one (tech in place) logs that option with `forced_ok` on every rep
- [x] Player builds contain no logging code for the new fields
- [x] `dev.py check` passes
