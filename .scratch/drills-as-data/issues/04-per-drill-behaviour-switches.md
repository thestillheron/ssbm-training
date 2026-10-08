# 04: Per-drill behaviour switches: pool, vanish, reset, facing

**What to build:** Each drill sets its tech option pool (any subset of the four options), whether the opponent vanishes, whether the player is reset each rep, and the opponent's facing (toward the player by default, or away, left or right). With reset off, the next rep chases from wherever the player ended up.

**Blocked by:** 01: Drill record and table, rep loop reads the current drill.

**Status:** resolved

- [x] Test drills with different option pools only ever produce options from their pool
- [x] Vanish on and off behave as set per drill
- [x] With reset player off, the player is not repositioned between reps
- [x] The opponent faces toward the player by default, and the away, left and right overrides work
- [x] All of these settings appear in the `drill_start` line
- [x] `dev.py check` passes

## Answer

`tech_chase.c` now reads all four switches from the current drill. Pool was already per drill (ticket 01). Vanish is implemented here per `.scratch/tech-vanish/spec.md`: the opponent is drawn for `VANISH_VISIBLE_FRAMES` (7) frames from landing, then `invisible` and `x221E_b1` (no shadow) are set every frame until its first vulnerable frame; never for missed tech; always shown again by the Outcome step. It logs `event=vanish vanish_from=.. vanish_to=..`. With reset player off only the first rep places the player (`player_placed`). Opponent facing is toward the player (from the player's side of the drop point, left on a tie), away, left or right. `drill_start` gains `facing=`; `drop` gains `opp_facing=` and `player_x=`.

To pick drills in acceptance scenarios, `dev.py build|run --drill N` (-> `configure.py --drill N` -> `TRAINING_DRILL_INDEX` in `drill.c`) selects table entry N, and authoring builds append test drills 5..9 (vanish, no reset, face away, left, right). Pool subsets use the existing `--tech-pool`.

Verified live: drill 5 (vanish) gave `vanish_from=38 vanish_to=50` for in and away (window_open 31, vuln_from 51), `vanish_to=54` for in place (vuln_from 55), and no vanish line for miss. Drills 7, 8, 9 (face away, left, right) gave `opp_facing` 1, -1, 1 with the player on the left. Drill 6 (no reset) with `tech-option-player-right` varied `player_x` (-11, 72, 166, 26, ...) with `opp_facing` flipping to 1 once the player stood right of the drop point. `--drill` unit tests are in `tools/dev_tests/test_authoring.py`. `dev.py check` passes.
