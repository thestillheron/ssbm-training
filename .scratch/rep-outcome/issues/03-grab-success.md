# 03: Grab success

**What to build:** A grab inside the punish window counts as a success, as grab punishes do in real play. The game doesn't credit grabs to the attacker slot, so a grab is detected by polling for the opponent entering a captured state whose grabber link is the player's fighter. A grab before the opponent lands is a void, like an early hit. The rep log records `hit_kind=grab`.

**Blocked by:** 02 (Hit success and void)

**Status:** resolved

- [x] A scenario that throws out grab attempts logs at least one success or failure line with `hit_kind=grab` where the grab connected (player timing is only checked loosely)
- [ ] A grab before landing is logged as void
- [ ] Grabs by anything other than the player's fighter are not credited
- [x] `dev.py check` passes

## Comments

Implemented in `src/training/tech_chase.c`. `poll_player_hit` now returns a kind (none, hit, grab). A grab is the frame the opponent newly enters a captured state (`CapturePulledHi` through `CaptureDamageLw`) whose `victim_gobj` (the grabber link on the captured fighter) is the player's GObj; grabs by any other fighter are not credited because that link must be the player's. Hit and grab go through the same success/void rules (window, void before landing, ignored in pause or after actionable). `outcome` lines carry `hit_kind=grab`. New scenario `tools/scenarios/rep-outcome-mash-grab.txt`; docs updated.

Verified live (`dev.py run --scenario rep-outcome-mash-grab`): `outcome=success ... hit_frame=81 hit_kind=grab` and `hit_frame=87 hit_kind=grab`, rep ends at once, the next rep starts clean, other reps stay `failure`. `dev.py check` passes.

Unverified live: a grab before landing logging void (a falling opponent in tumble cannot be grabbed by Ness's grab, so no scenario produces it; it shares the void path that ticket 02 verified for hits, only `hit_kind` differs), and that a grab by a non-player fighter is not credited (no way to produce one; by the `victim_gobj` check only). Status set to resolved with these two criteria left unticked.

Resolved. The two unticked criteria (grab-before-landing void; non-player grab not credited) could not be produced live: Ness in tumble cannot be grabbed, and there is no second grabber. They rest on code shared with the verified hit-void path and on the grabber-link (`victim_gobj`) check.
