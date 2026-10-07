# 04: Tech-chase rep emits the rep log

**What to build:** In authoring builds, each tech-chase rep writes rep log lines through the game's debug print for its events: rep start, drop (height), landing (frame), tech option and actionable (frame), each with a rep number and the game frame. Fields for the vulnerable window and outcome are left for rep-outcome. Player builds do no logging work and contain no rep log strings. After a live run, `dev.py reps` shows the lines.

**Blocked by:** 01: Authoring flag; 02: Spike: does OSReport reach Dolphin's log?

**Status:** resolved

**Note:** the live check takes Dolphin window focus, so ask the user first.

- [ ] Each rep writes rep start, drop, landing, tech option and actionable lines in the `[rep]` key=value format
- [ ] All emission is inside the authoring macro
- [ ] A guard test shows the training build without authoring contains no rep log strings, in the spirit of the existing unguarded-leak check
- [ ] A live run followed by `dev.py reps` shows at least two reps' lines

## Answer

Merged into authoring-tools-integration (final merge 84072fc42).
