# 01: One drill playable as an event

**What to build:** The tracer bullet for ADR-0005. In the training build, choosing the first row of Event Match plays curriculum drill 1. The path is 1P menu → Event Match → first event → player + CPU character select (Training Mode's CSS type) → a match on drill 1's stage, where the rep loop, the CPU takeover and the score run exactly as they do in Training Mode today. The event's name on the list can still be the vanilla one; ticket 02 builds the list. The retail pause still works for now; ticket 03 replaces it. Drills keep running in Training Mode too until ticket 06, so the existing scenarios stay green.

Follow the UnclePunch findings, "Mode entry, pause, restart and return to menu": the `GM_EVENT` route, CSS type 0x17, and match setup built from the drill in place of the event data file. Use one-line `TRAINING_BUILD` hooks calling into the training code, not address patches. Our event must not write Event Match's high scores or cleared bits to the save.

Add a boot-to-event scenario (1P → Event Match → first event → CSS → match) as the new prior art for drill scenarios.

**Blocked by:** None (can start immediately).

**Status:** ready-for-agent

- [ ] Choosing the first event leads through the player + CPU CSS to a match on drill 1's stage
- [ ] The rep loop runs drill 1: the rep log shows `drill_start` with drill 1's name, then reps with outcomes
- [ ] Our CPU input drives the opponent (ADR-0004), not the game's AI
- [ ] Finishing or quitting the event leaves Event Match's scores and cleared bits in the save unchanged
- [ ] A committed boot-to-event scenario reaches the drill, and its rep log confirms `drill_start`
- [ ] The existing Training Mode scenarios still pass
- [ ] The matching build is unchanged and `dev.py check` passes
