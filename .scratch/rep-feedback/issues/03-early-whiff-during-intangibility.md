# 03: Early: whiffed during intangibility

**What to build:** A failure where the player attacked into the opponent's intangibility is reported as early, with how many frames early. The player counts as having attacked while one of their hitboxes or grab boxes was active during the intangible part of the punish window (before the first vulnerable frame) and nothing connected. Report the frames between that attack and the first vulnerable frame. The rep log's outcome line gets `timing=early_Nf`, and the on-screen result's second line reads "Nf early" (white, under the red failure line from ticket 02). If detecting an active hitbox or grab box proves hard, fall back to the player being in an attack or grab state, and record which way was used in this ticket's comments. A failure with no early attack stays `none`.

**Blocked by:** 02 (rep result for success, none and void).

**Status:** ready-for-agent

- [ ] A failure with an attack or grab out during intangibility and no hit logs `timing=early_Nf`, with N the frames before the first vulnerable frame
- [ ] The on-screen result shows "Nf early" with the same N
- [ ] A new scenario that attacks right after a tech in place lands gives `early` (or a success if the hit lands in the vulnerable frames). This is a loose check, since piloting timing is approximate
- [ ] A shot of an early result matches its logged timing
- [ ] No-input reps still log `timing=none`
- [ ] `dev.py check` passes
