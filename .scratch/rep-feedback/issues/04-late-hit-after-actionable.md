# 04: Late: hit after actionable

**What to build:** A failure where the player's punish came too late is reported as late, with how many frames late. After a rep fails (the opponent became actionable unpunished), keep watching through the inter-rep pause. If a hit or grab by the player connects there, report the frames between the opponent's first actionable frame and the hit. The outcome stays a failure, and the score doesn't change. The rep log gets `timing=late_Nf`, and the on-screen result's second line changes to "Nf late". Because the late hit comes after the outcome is decided, either log the timing on its own line or write the outcome line at the end of the pause. Choose one and document it in the rep log comment. A failure with no late hit keeps `none` (or `early` from ticket 03).

**Blocked by:** 02 (rep result for success, none and void).

**Status:** resolved

- [x] A hit or grab that connects during the pause after a failure logs `timing=late_Nf`, with N the frames after actionable
- [x] The rep is still scored as a failure
- [x] The on-screen result shows "Nf late" with the same N
- [x] A new scenario that attacks very late gives `late` or `none`. This is a loose check, since piloting timing is approximate
- [x] A shot of a late result matches its logged timing
- [x] `dev.py check` passes

## Comments

- **Log-line choice:** the late hit gets its own `late` line, written when it connects during the pause, after the outcome line: `[rep] event=late ... actionable=<n> hit_frame=<n> hit_kind=hit|grab timing=late_Nf`, with N = `hit_frame - actionable` in rep frames (these now keep counting through Outcome and Pause). Its `timing` is the rep's final timing class and replaces the outcome line's. The outcome line stays where it was, so a restart during the pause never loses it. This is documented at `log_late` (tech_chase.c) and in the Rep log section of docs/developer-workflow.md. A hit on the actionable frame itself, which used to be dropped, is `late_0f` on both the outcome line and the late line.
- When a failure has both an early attack and a late hit, late wins, because the hit connecting is the stronger fact.
- Scenario `rep-late-hit`: every rep is a failure with `late_31f` to `late_33f`, score 0/10. Shot l03 ("32f late", score 0/3) matches rep 3's `late_32f`. In an earlier calibration run, l01 "8f late" matched `late_8f` and l09 "10f late" matched `late_10f`.
- Review: the actionable frame is no longer counted as vulnerable (see 02), so a hit on it being `late_0f` is now consistent with the window. `rep-late-hit` unchanged: late_31f..32f every rep.
