# 02: Custom Drill as a screen of its own

**What to build:** Custom Drill on the main menu opens the [custom drill](../../../docs/glossary.md#glossary_custom_drill)'s settings menu on the menu background, with no event list rows behind it ([spec](../spec.md)). The heading strip shows nothing new yet; ticket 05 adds "Custom Drill". Whether this reuses the event list scene with the list hidden or is a separate menu state is the implementer's choice, as long as that's what the player sees. The Custom drill row and its index past the last drill leave the event list, which now holds only the curriculum's drills.

Routes in and out:
- Start on the settings menu goes to the character select, stage select and match, as now.
- B on the settings menu returns to the main menu with the cursor on Custom Drill.
- B on the custom drill's character select returns to the settings menu.
- In a custom drill, the pause menu's back item reads "Settings" and returns to the settings menu.
- The custom drill's summary menu is Restart and Settings, with no Next drill. Settings returns to the settings menu.
- Custom settings last for the session through all of these, as now.

**Blocked by:** 01

**Status:** resolved

- [x] Choosing Custom Drill shows the settings menu with no drill rows behind it (shot).
- [x] The event list has no Custom drill row: its last row is the last curriculum drill (shot after scrolling to the bottom).
- [x] B on the settings menu returns to the main menu on Custom Drill.
- [x] Start, then character select and stage select, starts the drill: `drill_start` in the rep log carries the custom settings.
- [x] B on the character select returns to the settings menu.
- [x] The pause menu's "Settings" and the summary's "Settings" each return to the settings menu with the session's settings kept (scenario with rep log `event=menu` lines and a shot).
- [x] A custom drill's summary has no Next drill (shot).
- [x] The `custom-drill-*` and `custom-settings-*` scenarios go through the new route and pass.

## Comments

- **Implementation (for 04/05).** Custom Drill reuses the event list scene (`MENU_KIND_EVENT`, `mnEvent_8024E838`) opened on the Custom drill's index `training_drill_custom_index()` (= `DRILL_COUNT`), which is past the list: `training_drill_event_count()` is now `DRILL_COUNT`, so the list has only curriculum rows, and `training_event_drill_index()` accepts the custom index separately. The Custom Drill screen is simply "the event scene whose selection (`first_event + page`) is the custom index": `training_custom_settings_input` (custom_settings.c, called from `mnEvent_8024D864`) opens the settings menu on the first frame the scene exists and `training_event_list_hide` (event_list.c) hides the list's joints (1, 4, 6, 10, 12 of `MenMainConEv_Top`: arrows, cell column, description box, cursor, row marker) and its text. There is no flag: returning from a custom drill, its CSS (B, held about 40 frames) or the pause/summary "Settings" re-enters the scene on the stored custom event, so the settings menu reopens by itself. B calls `mn_80229894(MENU_KIND_MAIN, SEL_MAIN_VS, 3)`; Start sets `*inputs` to Confirm and the scene's normal confirm stores `first_event + page` (the custom index) as the event.
- **Heading still unchanged.** The heading strip is the retail `MenMainPanel` "1-P Mode / Event Match" (cur_menu 7, the panel's per-menu animation); ticket 05 replaces its two images. The panel also draws the (empty) list table cells at the left, and the description-box and "SELECT CHARACTER" outlines at the bottom; they cannot be hidden from the event scene's joints (the panel gobj is `mn_80229DC0`'s), so the settings menu is moved left (`MENU_X` 108 in custom_settings.c) to cover the cells. If 05 wants them gone it needs the panel's joints (cells under joints 5, 23, 40 of `MenMainPanel_Top`) or a different `cur_menu` animation.
- **Back labels.** `back_label()` (pause.c) says "Settings" in a custom drill; `show_summary_menu()` (tech_chase.c) swaps `summary_menu` between the three-item curriculum items and `custom_summary_items` (Restart, Settings; Settings is the old `SummaryChoice_EventList` exit).
- **Removed hooks.** `training_main_menu_take_custom_request` and the one-shot flag are gone.
- **Scenarios.** New `custom-screen-routes`, `custom-summary`. `custom-settings-persist` and `custom-drill-edit-layout` no longer press A on a list row after "Settings"; `drill-history-graph` lost its Custom-row tail; `event-list-scroll` ends on row 22. B on the character select needs a long hold (`b:40`); a 2 or 8 frame press did nothing. Not run: `custom-drill-player-pause` (`--no-authoring`), `drill-history-graph` after the trim, `new-save-*`.
