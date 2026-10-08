# 04: On-screen score

**What to build:** A running score is always on screen, drawn as text with the game's text library, as the title marker does. It shows the number of successes out of counted reps (for example `3/5`), and on another line the success rate as a percentage. Voids are not counted. Every outcome still goes to the rep log.

**Blocked by:** 01 (Punish window measured and logged)

**Status:** resolved

- [x] Line one shows successes out of counted reps; line two shows the success percentage
- [x] The score updates when a rep ends as a success or failure, and a void leaves it unchanged (see Comments)
- [x] The score is zero-safe before the first counted rep (no divide by zero)
- [x] A shot confirms the score text is drawn
- [x] `dev.py check` passes

## Comments

- Score lives in `src/training/score.c` (`training_score_init`, `training_score_show`, `training_score_record(bool success)`). `tech_chase.c` calls init on entering Training, show on the first Settle frame (text created during scene setup is not drawn), and `training_score_record(false)` in `finish_rep`. Ticket 02 should call `training_score_record(true)` for a success and nothing for a void.
- Verified live with `tools/scenarios/rep-outcome-score.txt`: shot shows `0/7` and `0%` after seven failed reps, and `0/0`, `0%` at the start.
- Not verified live: a success incrementing the count and percentage, and a void leaving the score unchanged. Success/void detection is ticket 02; here only `record(false)` is wired, and voids simply never call record. The percentage code is zero-safe (0 when no counted reps) and showed `0%` at `0/0`.
