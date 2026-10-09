# 05: Drill summary menu: Restart, Next drill, Event list

**What to build:** When a drill ends, the [drill summary](../../../docs/glossary.md#glossary_drill_summary) shows a menu built with the menu component:

- **Restart** restarts in place, as in the pause menu.
- **Next drill** plays the next drill in the table as the next event. It keeps both fighters and reloads the match scene, so the stage may change. It is disabled after the last drill.
- **Event list** works as in the pause menu.

Update the glossary's drill summary entry, which still says "Press A to restart the drill."

**Blocked by:** 04

**Status:** done

- [x] The summary shows the three items alongside the existing score, pass mark and per-option results (shot)
- [x] Restart starts the same drill from rep 1
- [x] Next drill starts the next drill with the same fighters, on its own stage when that differs (rep log `drill_start` name and stage)
- [x] Next drill is disabled (greyed and skipped) on the last drill
- [x] Event list reopens the list on the drill just played
- [x] Each choice writes a `menu` line
- [x] The glossary's drill summary entry matches the new behaviour
- [x] `dev.py check` passes

## Comments

- Summary menu is a static `TrainingMenu` in `tech_chase.c` (title "Drill over", items Restart / Next drill / Event list), shown when the summary state starts and driven with `Player_GetPadPort(0)`; the old A-restarts-directly code is gone. Menu text sits at x=30, y=160 (under the live score), the summary text moved right (x=265, table columns retuned) in `score.c`. `TrainingMenu` gained optional `x` and `y` (0 = default) for this.
- Next drill: `TrainingEventExit_NextDrill`; `training_event_exit_vs` sets `unk_535` to index+1, selects it and reloads the match with `gm_SetNextGameModeStateId(1)`, ahead of the retry and no-contest checks. Fighters carry over (file statics). Disabled when `training_drill_current_index() + 1 == training_drill_count()` (new accessor in `drill.h`).
- `start_drill` now clears `rep.press_pending` (cheap fix for the known restart edge; not separately reproduced).
- Added two authoring test drills at the end of the table: `Test: one rep` (FD) and `Test: one rep, last` (Battlefield), 1 rep each, so a scenario can reach Next drill (with a stage change) and the last-drill case. The authoring event list now has 14 drills (`event-list-scroll` comment updated).
- Scenario `drill-summary-menu`: drill 13, summary shot, Restart, Next drill (`drill_start` drill 14, stage=31), last-drill summary with Next drill greyed (down skips to Event list), Event list reopens on drill 14. The log shows a `menu menu="Drill over"` line per choice. Shots can lag the movie in the first drill (the first `summary` shot needed wait:520).
- `drill-end-no-input` still boots via `boot-to-training` (the old Training route); its A press still hits Restart, the first menu item. Left for ticket 06.
- `dev.py check` passes (matching, training, training-authoring).
