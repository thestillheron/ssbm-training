# 03: Missed tech

**What to build:** The opponent can miss the tech: no L/R press while airborne and no A/B (so the game doesn't go from bounce into a getup attack), it lands and lies down. After a fixed down time, shorter than the game's own auto-getup timer, it is fed a plain getup input (stick up) and gets up normally. Pick a down time that matches common play and record it in the spec's Implementation Decisions. A hit on the downed opponent (e.g. a jab reset) behaves as the game normally does, and the punish-window and success rules from rep-outcome apply to the missed-tech landing. The log reports `option=miss` with `result_state` knockdown and `forced_ok`.

**Blocked by:** 01 (Option choice and verified log)

**Status:** resolved

- [x] With a pool of one, missed tech logs `option=miss`, a knockdown `result_state` and `forced_ok` every rep (with the player on the left and, via `tech-option-miss-player-right`, on the right)
- [x] The opponent lies down for the fixed down time, then gets up with the plain getup; it never enters a getup attack
- [x] The chosen down time is recorded in the spec and defined with the pool in one place
- [x] A scenario that hits the downed opponent (`tech-option-miss-hit`, run live with `--tech-pool miss`: two successes with hit_frame 31 and 34 inside the window, 12 voids for hits before the landing, the rest failures at actionable=107) logs a success with the normal hit behaviour, and a hit before landing is still void
- [ ] Shots show the opponent lying down and then getting up (not verified in order: the shots at ~8-frame spacing did not clearly show lying down then getting up; the log's actionable frame (landing + 45 + 31) is the evidence)
- [x] `dev.py check` passes
