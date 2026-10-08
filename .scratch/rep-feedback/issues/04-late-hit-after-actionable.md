# 04: Late: hit after actionable

**What to build:** A failure where the player's punish came too late is reported as late, with how many frames late. After a rep fails (the opponent became actionable unpunished), keep watching through the inter-rep pause. If a hit or grab by the player connects there, report the frames between the opponent's first actionable frame and the hit. The outcome stays a failure, and the score doesn't change. The rep log gets `timing=late_Nf`, and the on-screen result's second line changes to "Nf late". Because the late hit comes after the outcome is decided, either log the timing on its own line or write the outcome line at the end of the pause. Choose one and document it in the rep log comment. A failure with no late hit keeps `none` (or `early` from ticket 03).

**Blocked by:** 02 (rep result for success, none and void).

**Status:** ready-for-agent

- [ ] A hit or grab that connects during the pause after a failure logs `timing=late_Nf`, with N the frames after actionable
- [ ] The rep is still scored as a failure
- [ ] The on-screen result shows "Nf late" with the same N
- [ ] A new scenario that attacks very late gives `late` or `none`. This is a loose check, since piloting timing is approximate
- [ ] A shot of a late result matches its logged timing
- [ ] `dev.py check` passes
