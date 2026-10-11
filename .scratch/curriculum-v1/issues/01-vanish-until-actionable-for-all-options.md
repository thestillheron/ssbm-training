# 01: Vanish until actionable, for all four tech options

Status: resolved

**What to build:** Change vanish (`update_vanish` and `end_vanish` in `src/training/tech_chase.c`) to the amended behaviour in [tech-vanish/spec.md](../../tech-vanish/spec.md). With a drill's vanish setting on, the opponent is drawn for the first 5 frames of its tech (counting from the landing frame), then hidden, model and shadow, until its first actionable frame, when it reappears. This applies to tech in place, tech in, tech away **and missed tech** (for a missed tech, until the frame on which it can choose its getup option). Today the hide ends at the first vulnerable frame and a missed tech is never hidden; both change. If the opponent is actionable at or before frame 6, nothing is hidden. The tech sound and flash are kept. The hide is still re-applied every frame, the reset at rep start still shows the opponent, and a hit or grab that closes the window still shows it. The rep log's `vanish_to` becomes `actionable` - 1, and a missed tech now logs vanish lines too. Update `docs/developer-workflow.md` and any scenario text that describes vanish as ending at the vulnerable frame or skipping a missed tech.

**Blocked by:** None (can start immediately).

**Status:** resolved

- [x] With vanish on, each of tech in place, tech in, tech away and missed tech logs `vanish_from` = landing + 5 and `vanish_to` = `actionable` - 1 (a missed tech: the frame before its getup is chosen)
- [x] With vanish off, no vanish lines are logged and the opponent is never hidden
- [x] The opponent and its shadow are hidden through the vanish window, with the hide re-applied every frame
- [x] The opponent is visible again on the actionable frame (the first frame after the punish window), and at the start of the next rep
- [x] A rep where the player hits or grabs the opponent mid-vanish shows it again when the rep ends, and the vanish never carries into the next rep
- [x] A missed tech is hidden from frame 6 until its getup choice frame, and is visible during the getup that follows
- [x] The tech sound and flash still play
- [x] The visible frame count is still a single named constant of 5
- [x] The `tech-vanish-shots` scenario is updated: a shot mid-vanish shows no opponent and no shadow, and one at the actionable frame shows the opponent (the log is the exact check, shots are visual)
- [x] `dev.py check` passes

## Comments

Needs Dolphin to verify (`dev.py run`), so ask the user before running it (focus permission), and `dev.py stop` afterwards.

Done on `curriculum-v1`. Live run (`--tech-pool all --scenario tech-vanish-shots`): every rep logs `vanish_from` = `window_open` + 5 (36 with `window_open` 31). Tech in and away log `vanish_to` = `actionable` - 1 (70). A missed tech is hidden from frame 6 until the frame its getup is chosen (`vanish_to` 75 = `window_open` + `MISSED_TECH_DOWN_TIME` - 1), so the getup that follows is visible; that is not `actionable` - 1 (106) because for a missed tech `actionable` is after the getup. This follows the spec and acceptance line 6 over line 1.
