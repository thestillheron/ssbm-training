# 04: Surface stepping onto platforms

**What to build:** The stick can move the landing point between surfaces, up and down steps such as ground to platform, snapped to the surface. Useful for platform drills.

**Blocked by:** 01

**Status:** resolved

- [x] Landing point steps between floors and platforms above and below its position
- [x] Shot: editor on Final Destination
- [x] Shot: editor on a stage with platforms, landing point on a platform

## Answer

A flick of the stick up or down (past 0.6 tilt, edge-triggered: one step per flick) moves the landing point to the next surface above or below it at the same x; with none (e.g. down on Final Destination) nothing happens, and a step the layout can't take is undone. `training_layout_step_surface` in `src/training/layout.c` finds the surface (down: cast from just under the current one; up: walk the floor casts down from high above, keeping the lowest surface above), `step_landing`/`update_step` in `layout_editor.c` apply it.

Scenario `layout-edit-step` (Battlefield, drill 4): `drill_start` lines read `landing_y=54`, `0`, `54`, then `landing_x=-40 landing_y=27` (left platform), with matching `layout` lines; shots `editor-platform`, `stepped-down`, `stepped-up`, `left-platform`. The Final Destination shot is `layout-edit-move`'s `editor-open` (that scenario still gives 60, 86, -74).
