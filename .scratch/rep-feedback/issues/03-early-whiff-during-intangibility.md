# 03: Early: whiffed during intangibility

**What to build:** A failure where the player attacked into the opponent's intangibility is reported as early, with how many frames early. The player counts as having attacked while one of their hitboxes or grab boxes was active during the intangible part of the punish window (before the first vulnerable frame) and nothing connected. Report the frames between that attack and the first vulnerable frame. The rep log's outcome line gets `timing=early_Nf`, and the on-screen result's second line reads "Nf early" (white, under the red failure line from ticket 02). If detecting an active hitbox or grab box proves hard, fall back to the player being in an attack or grab state, and record which way was used in this ticket's comments. A failure with no early attack stays `none`.

**Blocked by:** 02 (rep result for success, none and void).

**Status:** resolved

- [x] A failure with an attack or grab out during intangibility and no hit logs `timing=early_Nf`, with N the frames before the first vulnerable frame
- [x] The on-screen result shows "Nf early" with the same N
- [x] A new scenario that attacks right after a tech in place lands gives `early` (or a success if the hit lands in the vulnerable frames). This is a loose check, since piloting timing is approximate
- [x] A shot of an early result matches its logged timing
- [x] No-input reps still log `timing=none`
- [x] `dev.py check` passes

## Comments

- Detection: hitbox and grab box detection, not the attack-state fallback. A frame counts as an attack out when any of the player's four hitboxes (`Fighter.x914[]`) is not `HitCapsule_Disabled`. Grab boxes are those same hitboxes with `HitElement_Catch` (see ftcoll.c, which skips that element in the item hit loop), so grabs are covered without a separate check. The grab path has not been exercised live; only jabs have.
- N is measured from the *last* intangible window frame with a box out to the first vulnerable frame (`vuln_from`), so delaying by N frames would have put that box on the first vulnerable frame. Only window frames count (from the landing frame on); a whiff before landing stays `none`. If the opponent was never vulnerable, N is measured to `actionable`. A failure where the player attacked both early and on vulnerable frames, and nothing connected, still reports early.
- Scenario `rep-early-shots`: rep 1 has no input (`none`, "No attack"), then one jab per rep, 3 frames earlier each time. Run 2 logged early_3f, 6f, 8f, 11f, 14f, 17f and 20f for reps 2 to 8, and `none` for rep 9 (no input). Shots e02 ("3f early", score 0/2) and e05 ("11f early", 0/5) match the log. The first draft (cycle 177) locked on to `success_1/3`/`2/3` at frames 55 and 56, because a jab whose hitbox is still out at frame 55 connects there. That is the documented success case.
- Review: early now counts an attack or grab box out on any intangible window frame, not only before the first vulnerable one, with N measured to the next vulnerable frame (or the actionable frame if none follows). On a tech this is unchanged (`rep-early-shots` still early_3f..20f); on a missed tech a whiff into intangibility after vulnerable frames is now `early_Nf` instead of `none` (seen as `early_9f` on a miss rep with `--tech-pool all`).
