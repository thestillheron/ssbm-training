# 02: Tech in and tech away

**What to build:** The opponent can tech in (roll toward the player) and tech away (roll away from the player). Both are the tech-in-place press with the stick held fully left or right across the landing. Direction is in world terms: toward the player is the sign of (player x − opponent x), away is the opposite, so the option names match what the player sees whichever side they're on and whichever way the opponent faces. If the two x positions are equal, the opponent's facing counts as "toward". Rolls look and sound as they do against a human, and the log reports them as in or away (resolved from the game's forward/back roll), with `forced_ok`.

**Blocked by:** 01 (Option choice and verified log)

**Status:** resolved

- [x] With a pool of one, tech in logs `option=in`, the matching `result_state` and `forced_ok` every rep, with the player on the left and on the right of the landing point
- [x] With a pool of one, tech away does the same for `away`
- [ ] The equal-x case falls back to the opponent's facing as "toward" (both the forcing and the check use `side_of_player`; not verified live, equal x can't be hit reliably)
- [ ] Shots mid-animation show tech in moving toward the player and tech away moving away, from both sides (not verified: shots land about 8 frames apart, too coarse to read a roll's direction; the log's `forced_ok` is the evidence for direction)
- [x] The punish window and success rules still hold for rolls
- [x] `dev.py check` passes
