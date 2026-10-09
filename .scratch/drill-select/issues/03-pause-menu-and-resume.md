# 03: Our pause menu replaces the retail one (menu component, Resume)

**What to build:** During a drill event, Start freezes the game and opens our text pause menu instead of the retail pause. The menu has Resume, which unfreezes and continues the drill exactly where it was. It also shows the punish window length for the current opponent: a value measured on earlier reps this session, or "—" until one rep has measured it. Retail pause never runs in our events.

Build the reusable menu component here:

- items with a label, description, enabled flag and confirm callback
- a cursor
- a 9-row scrolling window that skips and greys disabled rows
- d-pad/stick with rapid-held input to move, A to confirm, B to go back (never unpauses), Start to close
- sounds 2, 1 and 0 for move, confirm and back
- the highlight shown by text colour

The UnclePunch findings give the pausing conventions and the freeze calls ("Pause"; "Recommendation for drill-select's in-drill controls"). Set `rules.disable_pausing`, then use the game's own pause flag, sound and HUD calls. Any human port's Start opens the menu, and the pausing port drives it.

The pause menu, the summary (ticket 05), layout-editor and custom-drills all use this component, so keep its interface small.

**Blocked by:** 01

**Status:** done

- [x] Start in a drill event freezes the match (fighters, CPU input, rep timers) and shows our menu; the retail pause UI does not appear (shot)
- [x] Resume continues the rep with no change to the score, clock or rep state (rep log)
- [x] B in the menu does not unpause; only Start or Resume does
- [x] The opponent's punish window is shown, or "—" before it has been measured
- [x] Menu selections write `menu` lines to the rep log
- [x] Scenario: start a drill → Start → shot → Resume → the drill carries on
- [x] `dev.py check` passes

## Comments

- Component: `src/training/menu.{h,c}`. A `TrainingMenu` (title, `TrainingMenuItem items[]` of label, description, enabled, `confirm`) is filled in by the owner; `training_menu_show/hide/reset` and `training_menu_update(menu, port)` once a frame, which returns `TrainingMenuAction_Back` (B) or `_Close` (Start); it never hides itself on those, and A runs the confirm callback (the callback may change `items[].enabled` or call `training_menu_hide`). It moves with `gm_801A36C0` rapid-held input (d-pad or stick), skips and greys disabled rows (grey text, highlight yellow), scrolls a 9-row window (checked with a temporary 12-item menu), plays sounds 2/1/0, and in authoring builds logs `menu menu=".." item=".."` (and `menu_back`). Call `training_menu_reset` at scene start (the match start does it). Layout constants are at the top of `menu.c`; the description sits under the last row.
- Pause: `src/training/pause.{h,c}`, started from `start_drill_match` in `event.c` (`training_pause_init`), which also sets `rules.disable_pausing` (`x3_4` L+R+A+Start stays on: retail's check still runs while our flag-1 pause is set). Start on any human port (after the Ready/GO) freezes with `lbAudioAx_80024E84(1)`, sound 5, `ifAll_802F3394`, `gm_SetDbPauseFlag(1)` and rumble pause; unpausing reverses them. The rep loop already stops on flag 1. Resume hides the menu and unfreezes once A is released (otherwise the A press leaks into the match as an attack).
- Punish window: `src/training/punish_window.{h,c}` caches the window length per opponent character and tech (`training_punish_window_record` from `finish_rep` in `tech_chase.c`, actionable minus landing, or predicted end on a hit), kept for the session. The menu shows "Punish window" and a line per tech in the drill's pool: `In place: 26f` or `In place: --`. `--` is seen only in the log (`punish_window=in-place:-1` when paused before rep 1 ended); my early-pause shot attempt lost its timing, so that one rendering is not verified by shot.
- Review fix: the placeholder stays "--", not "—": the font conversion in `text.h` is ASCII only. Verified by shot (`pause-before-measure`) that `--` renders in the pause menu before any rep has measured the window. The same lines are also shown at drill start (see `punish_window_panel_*`).
- Rep log: `pause` and `resume` lines (`training_tech_chase_log_progress`) carry rep, frame, rep state, frames in state, rep frames, drill clock and score. `event-pause-resume` shows them identical for each pause (rep 4: frame 834, drill_frames 691, score 0/4, by=resume; rep 7: frame 1294, by=start), no `late` line from the pause, and reps carrying on.
- `dev.py check`: matching, training and training-authoring PASS.
- For ticket 04: add items to `items[]` in `pause.c` (Restart drill, Change characters, Back to event list), each with a `confirm`; to unpause from a callback set `resume_chosen`-style flag as `choose_resume` does (or call `unpause` after leaving the frozen state; ending the match from a callback should also clear flag 1 if a new scene does not). Restart should call `start_drill` in `tech_chase.c` (static: expose a function) and then unpause. For ticket 05: own a static `TrainingMenu` with `training_menu_reset` at drill start, show it on the summary, drive it with the player's port.