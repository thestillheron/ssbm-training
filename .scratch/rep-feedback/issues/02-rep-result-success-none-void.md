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

**Status:** ready-for-agent

- [ ] The inter-rep pause is 120 frames
- [ ] Each counted rep's outcome line logs `timing=success_k/n` or `timing=none`, and a void logs `timing=void`
- [ ] The no-input scenario logs `timing=none` on every rep
- [ ] A mash-attack scenario logs a success with k and n (or a void), and k is within 1..n
- [ ] The result is shown above the player's damage HUD for the whole pause, coloured by outcome, and cleared at reset
- [ ] The old "early" notice is gone, and a void shows the new result instead
- [ ] Shots of a success, a none and a void, each matching its logged timing (the shots must match the log exactly)
- [ ] `dev.py check` passes
