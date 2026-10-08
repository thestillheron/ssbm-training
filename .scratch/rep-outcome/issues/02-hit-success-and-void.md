# 02: Hit success and void

**What to build:** The player's hit on the opponent is detected by polling each frame: the opponent took damage this frame and the recorded attacker is the player's slot. A hit inside the punish window is a success and ends the rep at once; the opponent is then left to normal hitstun with neutral fed input until the inter-rep pause ends. A hit before the opponent lands is a void: it doesn't count, "early" is shown, and the rep re-drops after the pause. Whiffs (including attacks during the tech's intangibility) don't end the rep. A hit after the opponent is actionable, or during the inter-rep pause, is ignored. Damage from anything other than the player is not credited. The rules must already hold for a missed-tech landing (a hit while the opponent lies on the ground or gets up counts until it is actionable), though only tech in place can be produced for now. The rep log gains `hit_frame` and `hit_kind=hit`, with `outcome` success/void.

**Blocked by:** 01 (Punish window measured and logged)

**Status:** resolved

- [x] A scenario that mashes an attack during the fall logs voids with the frame of the early hit
- [x] A hit inside the window logs `outcome=success` with `hit_frame` and `hit_kind=hit`, and the rep ends immediately
- [x] A void shows "early", is not counted, and re-drops after the pause
- [x] Whiffs, hits after actionable, and hits during the pause don't change the outcome
- [x] Only hits attributed to the player's slot count
- [x] The window and success rules are not specific to tech in place, so they hold for missed-tech landings
- [x] `dev.py check` passes

## Comments

Implemented in `src/training/tech_chase.c`. The hit is polled every frame (`poll_player_hit`): the opponent's damage percent rose and `dmg.x18c4_source_ply` (the recorded attacker slot) is the player's slot. A hit in the Drop/Wait states is a void (logged `outcome=void` with `hit_frame`, `rep.voided` shows "early" through the pause, then the normal pause and re-drop); a hit on or after the landing frame and before the opponent is back in Wait is a success (`outcome=success`, `hit_kind=hit`, rep ends at once, neutral input until the pause ends). Hits in the pause or after actionable are ignored because those states do not read the poll. The rules use the landing/actionable measurement of ticket 01, so they are not specific to tech in place. Failure lines now also carry `hit_frame=-1 hit_kind=none`. Success calls `training_score_record(true)`, failure `false`, void nothing (ticket 04's score).

Verified live (`dev.py run --scenario rep-outcome-mash-attack`, repeated, deterministic): `outcome=success ... hit_frame=31/50-54 hit_kind=hit` and `outcome=void window_open=-1 ... hit_frame=30 hit_kind=hit` lines; reps without a connecting hit stay `outcome=failure`; the rep after a void starts normally. The score shot showed 1/7, 1/8, 1/9 across a success, a void and failures, so the void left it unchanged. `dev.py check` passes.

Unverified: a shot of the "early" text during a void pause. The text renders (seen at creation), and toggling it in the pause was seen, but the Dolphin screenshots folder is shared between Dolphin instances, so with other agents running the scenario `rep-outcome-early-shots` produced foreign frames. Missed-tech and grab hits are not producible yet (ticket 03, tech-options); the window rules are shared.

Follow-up (ticket 03 branch): the "early" text during the pause after a void was verified live with a clean single-Dolphin run: with the void pause temporarily lengthened (to 900 frames, reverted), shots in the pause show "early" with the score unchanged (1/7). With the normal 60-frame pause the scenario's burst shots mostly miss it (shot spacing is wall-clock and uneven), so use a long pause to check it by eye. Nothing was broken.
