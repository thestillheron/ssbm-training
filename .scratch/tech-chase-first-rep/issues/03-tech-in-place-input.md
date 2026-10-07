# Feed the tech-in-place press

Status: resolved
Blocked by: 02
Spec: ../spec.md (stories 3, 4, 12)

## What to build

In the *airborne* state, feed exactly one L (or R) press with neutral stick, timed so the landing falls inside the game's tech window (and respecting the post-press lockout). Time it from a per-frame landing prediction (height above floor and fall speed); a fixed delay from the drop is an acceptable fallback. Window and lockout lengths come from runtime game data. The tech must be produced by input, not by calling state functions.

## Acceptance criteria

- [ ] The opponent techs in place on every rep, with the real animation, sound, flash and face-up/face-down variants.
- [ ] Exactly one press per rep; no lockout-induced misses across a run of many reps and several characters.
- [ ] Shots show the tech mid-animation.
