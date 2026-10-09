# 02: Rep result for success, none and void, logged and on screen

**What to build:** The tracer bullet for rep feedback. After each rep, the rep log's outcome line gets a `timing` field, and the player sees a short two-line result during the inter-rep pause. This ticket covers the timing classes that existing rep-outcome frames already decide:

- **Success:** the hit or grab landed on vulnerable frame k of n, logged as `timing=success_k/n`, with frames counted from 1.
- **None:** a failure with no attack during the punish window, logged as `timing=none`.
- **Void:** a hit before landing, logged as `timing=void`.

The on-screen result follows the UnclePunch research (`.scratch/unclepunch-ui-research/findings.md`, "Recommendation for rep-feedback"):

- It is placed above the player's damage HUD, anchored at `ifAll_GetPlayerHUDPosition` plus a small upward offset, and centred, so it doesn't hide the fighters.
- Line 1 shows the tech option and the outcome, coloured soft green for a success, red for a failure and yellow for a void. Start from UnclePunch's RGB values and tune by eye.
- Line 2 shows the timing in white: "Frame 2/6", "No attack", or "Early: hit before landing".
- It replaces the existing "early" notice for a void rep.

**Duration (decided by the developer):** the inter-rep pause grows from 60 to 120 frames, and the result shows for the whole pause and clears at reset. This replaces the research's suggestion of a message that lives past the reset: with the longer pause the two come to the same thing. The spec's "cleared at reset" decision stands.

**Blocked by:** 01 (shared text helper).

**Status:** resolved

- [x] The inter-rep pause is 120 frames
- [x] Each counted rep's outcome line logs `timing=success_k/n` or `timing=none`, and a void logs `timing=void`
- [x] The no-input scenario logs `timing=none` on every rep
- [x] A mash-attack scenario logs a success with k and n (or a void), and k is within 1..n
- [x] The result is shown above the player's damage HUD for the whole pause, coloured by outcome, and cleared at reset
- [x] The old "early" notice is gone, and a void shows the new result instead
- [x] Shots of a success, a none and a void, each matching its logged timing (the shots must match the log exactly)
- [x] `dev.py check` passes

## Comments

- Classification is `classify_timing` in `src/training/tech_chase.c`. The timing type, its log value, its on-screen text and the result text itself are in `src/training/rep_result.{h,c}`. The outcome line gains `timing=` and `pred_vuln_to=`.
- n on a success: the window ends at the hit, so n is predicted from the opponent's animation (the frames left in its tech or getup animation, `predict_vuln_to`). On every failure logged, `pred_vuln_to` equalled the measured `vuln_to` (tech in place 57, missed tech 107), so a success's n matches a failure's. A missed tech hit before its getup uses the last missed tech's measured window. If no missed tech has run to actionable yet in the session, n is unknown and logs as `success_k/0` ("Frame k" on screen).
- Live checks: `rep-outcome-no-input` logs `timing=none` on all 10 reps. The new `rep-result-shots` logs rep 1 `none`, rep 2 `void` and rep 3 `success_2/3` (hit_frame 56, vulnerable 55..57), the same on two runs. Its shots `none`, `r01` and `r03` show "Tech in place: Failure / No attack" (red), "Tech in place: Void / Early: hit before landing" (yellow) and "Tech in place: Success / Frame 2/3" (green), each with the matching score. With `--tech-pool miss` it logs `success_15/77`, shown as "Missed tech: Success / Frame 15/77".
- The score text moved from above the damage HUD to the top left, under the Training Mode logo, because the result now sits where it was.
- Review (off by one): the glossary's punish window ends before the first actionable frame (a hit on it is late), so the actionable (Wait) frame is no longer counted as vulnerable. `vuln_to` and `pred_vuln_to` are now `actionable - 1` (tech in place 55..56, so n=2; `rep-result-shots` rep 3 is now `success_2/2`, missed tech n=76), and k=n is reachable.
- Review (k within 1..n): the missed-tech window length is now learnt from any missed tech that reaches its getup (success or failure, from the getup animation), kept per opponent character for the whole boot, not just from the previous miss failure in the session. `k/0` remains only for a missed tech hit before its getup before any missed tech has reached one (the getup's animation is not loaded until it starts, so its length cannot be read earlier); a prediction smaller than k is treated as unknown, so k > n is never shown.
- Review (structure): classification is now `training_rep_classify(const RepFrames*)` in `rep_result.c`; RepData holds the measured frames as one `RepFrames` (with `PlayerHit hit` and `late_hit` structs). The tech, outcome and timing names are one table each in `rep_result.c` (labels, summary labels, log names and line-2/log formats), `tech_chase.c` has one TechResult table (tech plus `result_state` name), and the result's longest line is computed from those strings instead of the hard-coded 25. Rep log names and `training_rep_timing_log_value` are authoring-only (`REP_LOG_ONLY`). `rep_frames` is counted in one place in `think`.
