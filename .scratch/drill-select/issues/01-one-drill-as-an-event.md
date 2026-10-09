# 01: One drill playable as an event

**What to build:** The tracer bullet for ADR-0005. In the training build, choosing the first row of Event Match plays curriculum drill 1. The path is 1P menu → Event Match → first event → player + CPU character select (Training Mode's CSS type) → a match on drill 1's stage, where the rep loop, the CPU takeover and the score run exactly as they do in Training Mode today. The event's name on the list can still be the vanilla one; ticket 02 builds the list. The retail pause still works for now; ticket 03 replaces it. Drills keep running in Training Mode too until ticket 06, so the existing scenarios stay green.

Follow the UnclePunch findings, "Mode entry, pause, restart and return to menu": the `GM_EVENT` route, CSS type 0x17, and match setup built from the drill in place of the event data file. Use one-line `TRAINING_BUILD` hooks calling into the training code, not address patches. Our event must not write Event Match's high scores or cleared bits to the save.

Add a boot-to-event scenario (1P → Event Match → first event → CSS → match) as the new prior art for drill scenarios.

**Blocked by:** None (can start immediately).

**Status:** done

- [x] Choosing the first event leads through the player + CPU CSS to a match on drill 1's stage
- [x] The rep loop runs drill 1: the rep log shows `drill_start` with drill 1's name, then reps with outcomes
- [x] Our CPU input drives the opponent (ADR-0004), not the game's AI
- [x] Finishing or quitting the event leaves Event Match's scores and cleared bits in the save unchanged
- [x] A committed boot-to-event scenario reaches the drill, and its rep log confirms `drill_start`
- [x] The existing Training Mode scenarios still pass
- [x] The matching build is unchanged and `dev.py check` passes

## Comments

- Code: `src/training/event.c` (+ `event.h`), called from five one-line `TRAINING_BUILD` hooks in `src/melee/gm/gmevent.c`: `gm_Mode_Event_OnLoad` (end: `training_event_on_load`, forces the CSS even for fixed-character events), `onEnterCss` (`training_event_enter_css`, CSS type 0x17), `onExitCss` after retail's B-back check (`training_event_exit_css`, keeps player and CPU picks in `event.c` statics), `onEnterVs` (`training_event_enter_vs`, builds `StartMeleeData` from the drill, `on_match_start` = our start, which calls `training_tech_chase_init`), `onExitVs` after `gm_8016A164` (`training_event_exit_vs`: retry reloads the match, anything else `GM_MENU` = event list; none of retail's score/cleared/stats writes run). Each returns false for a vanilla event.
- `training_event_drill_index()` is the single event-to-drill mapping (event 0 -> drill 0 for now); ticket 02 should replace it with the row-to-drill mapping and hook the ESS text.
- Match rules: endless time match, no items, retail pause with Z retry and L+R+A+Start prompts (ticket 03 replaces it; set `disable_pausing` there). The rep loop now also stops while `gm_GetDbPauseFlag(1)` (VS pause) is set.
- VS matches have a Ready/GO countdown with fighter input disabled; the rep loop's settle now waits for input to be enabled, otherwise rep 1's tech press was dropped. Training Mode is unaffected (rep 1 still at loop frame 61).
- Save check: the `event_save` rep log line (authoring) at every match start and exit; in `event-drill-first-reps` all lines read `cleared=0007ffffffffffff best0=1834 best_sum=233999` across reps, a Z retry and a quit, and the event list still shows Trouble King's 00:30.57.
- Regression runs: `tech-option-pool-of-one`, `drill-chooser`, `drill-end-no-input` give the same logs as before. Pre-existing, not from this ticket: `drill-end-no-input`'s A press lands before the summary appears (drill_end at loop frame 1850), so its restart is no longer exercised; its timing needs retuning.
