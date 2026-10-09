# 02: Event list built from the drill table

**What to build:** The [event list](../../../docs/glossary.md#glossary_event_list) shows the drill table and nothing else. Each drill is its own event, numbered in curriculum order, with its name on the row and its description and pass mark in the description box. The vanilla events are hidden, and the list scrolls and pages only over our drills. Test drills appear in authoring builds only. Choosing any event plays that drill on its own stage, with the characters picked at the CSS. Picking an event writes a `menu` line to the rep log.

Text goes through the shared ASCII-to-SIS helper (rep-feedback ticket 01, if it has landed; otherwise extend the training code's existing conversion). The UnclePunch findings, "Menus" ("Event selection: the retail Event Match screen, patched"), list the event select screen functions to hook.

**Blocked by:** 01

**Status:** done

- [x] The list shows exactly the drill table's drills in order, numbered, and no vanilla event
- [x] Highlighting a drill shows its description and pass mark
- [x] Each event starts its own drill on its own stage (rep log `drill_start` name and stage)
- [x] Test drills are listed in authoring builds and absent from player builds
- [x] The rep log has a `menu` line naming the chosen drill
- [x] Scenario: Event Match → choose drill 2 → `drill_start` for drill 2, with a shot of the list
- [x] Adding a drill to the table adds an event with no other change
- [x] `dev.py check` passes

## Comments

- Code: `src/training/event_list.c` (+ `.h`), called from four one-line `TRAINING_BUILD` hooks in `src/melee/mn/mnevent.c` (`mnEvent_8024CE74` last top row, `mnEvent_8024D15C` row builder, `mnEvent_ShowSelected` description, Down in `mnEvent_8024D864` stops at the last drill). `training_event_drill_index()` in `event.c` is now just the list position (the stored event `unk_535`) when it is below the drill count; the screen's stored event is the drill index, so reopening the list on the last event (ticket 04) already lands on the drill just played (`mnMain` calls `mnEvent_8024E838(gm_801BEB80(), false)`, which opens at that index; `first_event`/`page` are derived from it by `mnEvent_8024E420`, which uses `last_top`, now `count-9` or 0).
- Rows are plain SIS texts (`HSD_SisLib_803A6754`, text.h's ASCII-to-SIS helper); `HSD_SisLib_803A5ACC` texts crash (null read at 0x803a6124) when created for ASCII-converted lines here, so use `803A6754` plus pos fields. Retail's cleared icon, score and portrait are not drawn; the score-kind label joint is hidden. Description wraps at 32 chars, 30 units per line, pass mark on the last line (max 3 lines).
- `menu` log line: `event=menu menu=event_list choice=drill drill=<n> name="..."`, logged from `training_event_on_load` (the point where Event Match loads after A on the list), authoring only. Scenarios: `event-list-choose-second`, `event-list-scroll` (new prefix `boot-to-event-list`, fragment `event-css-to-match`; `boot-to-event` expands identically to before). Drill 4 (Battlefield) checked to start with stage=31.
- Player build (`--no-authoring`): the list shows only drill 1 (shot). Cursor does not move in a player build as there is one row.
- Shots land a few frames late: `event-list` in the choose-second run already shows row 2 highlighted.
- Notes for 04: the shortcut above; `last_top` means the list opens with the played drill visible. Dolphin pad presses on the list need about 14 frames spacing.
