# 01: Vanish for tech in place, tech in and tech away

**What to build:** When a drill's vanish setting is on, the opponent is drawn normally for the first 7 frames of its tech (counting from the landing frame), then hidden, model and shadow, until its first vulnerable frame, when it reappears. This applies to tech in place, tech in and tech away, for any opponent character, using the vulnerable frame measured by rep-outcome. If the vulnerable frames start at or before frame 8, nothing is hidden. A missed tech is never hidden. The tech sound and flash are kept. The game clears the invisible and no-shadow flags on every motion state change, so they are set again every frame while hidden. The reset at rep start always shows the opponent, so a vanish never carries over. The visible frame count is one named constant of 7, not a drill setting. Vanish is off by default. The rep log gets `vanish_from` (landing + 7) and `vanish_to` (`vuln_from` - 1) lines, and none for a missed tech or when vanish is off.

**Blocked by:** None (can start immediately). Uses the vanish bool from drills-as-data ticket 01; ticket 04 there lets drills set it.

**Status:** resolved

- [x] With vanish on, each of tech in place, tech in and tech away logs `vanish_from` = landing + 7 and `vanish_to` = `vuln_from` - 1
- [x] A missed tech logs no vanish lines and is never hidden
- [x] With vanish off, no vanish lines are logged and the opponent is never hidden
- [x] The opponent and its shadow are hidden through the vanish window, with the hide re-applied every frame
- [x] The opponent is visible again at the first vulnerable frame, and at the start of the next rep
- [x] The tech sound and flash still play
- [x] The visible frame count is a single named constant of 7
- [x] A shot taken mid-vanish shows no opponent and no shadow, and one at the vulnerable frame shows the opponent (the log is the exact check, shots are visual)
- [x] The timing is correct for more than one opponent character
- [x] `dev.py check` passes

## Comments

The behaviour (`update_vanish`, `end_vanish`, `VANISH_VISIBLE_FRAMES` in `src/training/tech_chase.c`) already landed with drills-as-data (`cd6e10dec`); this ticket verified it. `python dev.py run --drill 5 --scenario tech-option-pool-of-one`: every in-place, in and away rep logs `vanish_from=38` (window_open 31 + 7) and `vanish_to=vuln_from-1` (54 for in-place, 50 for in and away); miss reps log no vanish line. New scenario `tech-vanish-shots` (`run --drill 5 --tech-pool in-place --scenario tech-vanish-shots`): shot v12 is mid-vanish with no opponent and no shadow and the tech flash still drawn; the next rep (v13) has the opponent visible again. A shot exactly on the 3-frame vulnerable window was not caught (shot timing is approximate); the log is the exact check. `dev.py check` passes (matching, training, training-authoring).

Not verified: a second opponent character. Training keeps one CPU character per boot (Pichu here) and the timing uses live-measured frames with no per-character tables, so it holds by construction.
