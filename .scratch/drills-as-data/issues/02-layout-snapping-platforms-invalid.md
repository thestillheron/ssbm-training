# 02: Layout snapping, platforms and invalid layouts

**What to build:** The landing point and the player's start offset are snapped onto the surface below them by a downward ray-cast with the stage collision library's floor check, at drill start and at each reset. The landing point can be on the main floor or a platform, so "opponent on the platform, player on the ground below" works. If the ray-cast misses for either point, the layout is invalid: the drill reports it clearly in the rep log and on screen, refuses to run, and does not crash.

**Blocked by:** 01: Drill record and table, rep loop reads the current drill.

**Status:** resolved

- [x] The landing point snaps to the floor or platform surface chosen by its y hint
- [x] The player start is placed at the stored offset from the landing point and snapped to the floor below that spot
- [x] A test drill with a platform layout on a stage with platforms has the opponent landing on the platform (verified by a screenshot)
- [x] A deliberately bad test drill (authoring builds only) logs an invalid-layout error, shows it on screen and does not run
- [x] `dev.py check` passes

## Answer

Added `src/training/layout.c/.h`: `training_layout_resolve` snaps a drill's layout with `mpCheckMultiple` (floors only) cast straight down, from the landing point's y hint (so the hint picks main floor or platform) and then from the player start (landing x minus the fighters' reach plus `start_dx`, y = landing surface y plus `start_dy`). `tech_chase.c` resolves the layout at every reset instead of using the stage floor centre; the opponent drops over the snapped landing surface and the player teleports onto the snapped start. A miss logs `[rep] event=invalid_layout drill="..." problem=...`, draws "Invalid layout" plus the problem and drill name, and parks the rep loop in a new `RepState_Invalid` (no reps, no crash). A `layout` rep-log event (landing and start x/y) follows each `start`.

For testing, `--drill N` (from ticket 03) picks the current drill by table index, and two authoring-only test drills were added: 3 "Test: platform" (Battlefield top platform, player below) and 4 "Test: invalid layout" (landing x far off stage). New scenarios `boot-to-training-battlefield`, `drill-platform-layout`, `drill-invalid-layout`. Verified live: drill 3 logs `landing_y=54 start_y=0` and the shots show the opponent landing on the top platform with the player on the floor; drill 4 logs `invalid_layout problem=no-floor-below-landing`, shows it on screen and runs no reps; the default drill still logs `landing_y=0 start_y=0` and `tech-option-pool-of-one` reps are unchanged. Note: the player start x keeps the old reach-based adjacency (`start_dx` is an offset beyond it).
