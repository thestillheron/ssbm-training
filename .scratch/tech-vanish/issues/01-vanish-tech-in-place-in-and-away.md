# 01: Vanish for tech in place, tech in and tech away

**What to build:** When a drill's vanish setting is on, the opponent is drawn normally for the first 7 frames of its tech (counting from the landing frame), then hidden, model and shadow, until its first vulnerable frame, when it reappears. This applies to tech in place, tech in and tech away, for any opponent character, using the vulnerable frame measured by rep-outcome. If the vulnerable frames start at or before frame 8, nothing is hidden. A missed tech is never hidden. The tech sound and flash are kept. The game clears the invisible and no-shadow flags on every motion state change, so they are set again every frame while hidden. The reset at rep start always shows the opponent, so a vanish never carries over. The visible frame count is one named constant of 7, not a drill setting. Vanish is off by default. The rep log gets `vanish_from` (landing + 7) and `vanish_to` (`vuln_from` - 1) lines, and none for a missed tech or when vanish is off.

**Blocked by:** None (can start immediately). Uses the vanish bool from drills-as-data ticket 01; ticket 04 there lets drills set it.

**Status:** ready-for-agent

- [ ] With vanish on, each of tech in place, tech in and tech away logs `vanish_from` = landing + 7 and `vanish_to` = `vuln_from` - 1
- [ ] A missed tech logs no vanish lines and is never hidden
- [ ] With vanish off, no vanish lines are logged and the opponent is never hidden
- [ ] The opponent and its shadow are hidden through the vanish window, with the hide re-applied every frame
- [ ] The opponent is visible again at the first vulnerable frame, and at the start of the next rep
- [ ] The tech sound and flash still play
- [ ] The visible frame count is a single named constant of 7
- [ ] A shot taken mid-vanish shows no opponent and no shadow, and one at the vulnerable frame shows the opponent (the log is the exact check, shots are visual)
- [ ] The timing is correct for more than one opponent character
- [ ] `dev.py check` passes
