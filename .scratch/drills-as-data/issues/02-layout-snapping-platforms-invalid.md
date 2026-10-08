# 02: Layout snapping, platforms and invalid layouts

**What to build:** The landing point and the player's start offset are snapped onto the surface below them by a downward ray-cast with the stage collision library's floor check, at drill start and at each reset. The landing point can be on the main floor or a platform, so "opponent on the platform, player on the ground below" works. If the ray-cast misses for either point, the layout is invalid: the drill reports it clearly in the rep log and on screen, refuses to run, and does not crash.

**Blocked by:** 01: Drill record and table, rep loop reads the current drill.

**Status:** ready-for-agent

- [ ] The landing point snaps to the floor or platform surface chosen by its y hint
- [ ] The player start is placed at the stored offset from the landing point and snapped to the floor below that spot
- [ ] A test drill with a platform layout on a stage with platforms has the opponent landing on the platform (verified by a screenshot)
- [ ] A deliberately bad test drill (authoring builds only) logs an invalid-layout error, shows it on screen and does not run
- [ ] `dev.py check` passes
