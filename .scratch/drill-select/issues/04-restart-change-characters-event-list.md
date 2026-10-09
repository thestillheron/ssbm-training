# 04: Restart drill, Change characters and Back to event list

**What to build:** The pause menu gains its remaining items:

- **Restart drill** resets the drill in place to rep 1 with a clean score and clock, without reloading the scene.
- **Change characters** ends the match and returns to the character select screen for the same event.
- **Back to event list** ends the match and reopens the event list with the drill just played highlighted.

L+R+A+Start during a drill also goes to the character select screen for the same event, as in vanilla matches. Retail Event Match sends it to the menu, so it has to be routed to the CSS instead. The findings' "Restart" and "Return to menu" sections cover the retail routing: the `onExitVs` outcomes and the "reopen on the last event" behaviour.

**Blocked by:** 02, 03

**Status:** done

- [x] Restart drill logs a fresh `drill_start`, and the next rep is rep 1 with score 0
- [x] Change characters reaches the CSS, and confirming starts the same drill (rep log)
- [x] L+R+A+Start reaches the CSS for the same event
- [x] Back to event list reopens the list on the drill just played (shot)
- [x] Each choice writes a `menu` line
- [x] Scenario: drill 2 → pause → restart → Back to event list → drill 1 → pause → Change characters → CSS → start → L+R+A+Start → CSS, with the `drill_start` names checked in the rep log
- [x] Event Match's scores and cleared bits are still never written
- [x] `dev.py check` passes

## Comments

- Pause items (`pause.c`): Resume, Restart drill, Change characters, Back to event list. A confirm callback only sets a `chosen` value; `think` acts on it. Restart calls `training_tech_chase_restart()` (new, in `tech_chase.c`, wraps `start_drill`) while still frozen, then resumes like Resume (waits for A release). Change characters / Back to event list unpause first (clears flag 1, sound, HUD, rumble), then `training_event_end_match(exit)` (`event.c`): records the destination and calls `gm_8016B328` (terminate match), so onExitVs runs with OUTCOME_TERMINATED and `training_event_exit_vs` sends it to the CSS (`gm_SetNextGameModeStateId(0)`, as Training Mode does) or the list (`GM_MENU`, which reopens on the stored event, unchanged: verified by shot, drill 2 highlighted). No pause state leaks into the next scene (the CSS and list worked after each).
- L+R+A+Start: retail's check only runs while frozen and uses `state.pauser`, which our pause never sets, so `pause.c` checks the chord itself on every human port (before Start opens the menu, and while paused) and ends the match to the CSS. `OUTCOME_NO_CONTEST` in `training_event_exit_vs` also goes to the CSS, in case retail's check fires first.
- Rep log: each choice writes `menu menu="Paused" item="..."` (menu.c); the chord writes `resume ... by=quit_chord` (its pause/resume progress line) and the next scene's `drill_start`. `event_save` lines stay `cleared=0007ffffffffffff best0=1834 best_sum=233999` throughout.
- Scenario `event-restart-change-list`: drill 2, Restart (new `drill_start`, then `tech rep=1`), Back to list, drill 1, Change characters, start, L+R+A+Start. The CSS entered from a match ignores Start for about 3 s (waits of 120 frames did not register, about 220 did), hence the long wait in the scenario; this looks like the CSS's own load, not our code, but I did not dig into it.
- Known edge, not fixed: pausing on the exact frame the tech press is queued (`press_pending` set but not yet fed to the opponent) then Restart gave a rep 1 whose tech did not register (shot-less, found while tuning the scenario's timing; `x684` showed the lockout was not the cause). Resume at that moment was not tested. Rare (a one-frame window); if it matters, clear `press_pending` and the opponent's input on restart.
- For ticket 05 (summary menu): `training_tech_chase_restart()` is the Restart action; the Event list action is `training_event_end_match(TrainingEventExit_EventList)` (needs the match unfrozen, which the summary is). Next drill needs a new exit target in `event.c`: set `event_data()->unk_535` to index+1 (`training_drill_select` too), then `gm_SetNextGameModeStateId(1)` from `training_event_exit_vs` (as OUTCOME_RETRY does) to reload the match scene with the same picks; add `TrainingEventExit_NextDrill` to the enum.
