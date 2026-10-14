# 01: Four-item main menu and the RTC 101 route

**What to build:** In the training build, every build included, the main menu's top level is RTC 101, Custom Drill, Options and Data ([spec](../spec.md), [ADR-0007](../../../docs/adr/0007-training-build-owns-the-main-menu.md)). The four sit together with no gap, and the cursor starts on RTC 101. 1P Mode, VS Mode and Trophies are gone: the cursor can't reach them. Labels, descriptions and preview panels stay retail for now; tickets 03 and 04 replace them. This ticket only fixes which selection is which.

RTC 101 opens Event Match's [event list](../../../docs/glossary.md#glossary_event_list) as 1P Mode → Event Match does now. Leaving the list, from the list itself or after returning from a drill, lands on the main menu with the cursor on RTC 101, not in the 1P submenu. Custom Drill, for now, opens the event list straight into the Custom drill row's settings menu, so the custom drill stays reachable. Ticket 02 gives it a screen of its own. Options and Data behave exactly as in retail.

In a curriculum drill, the [pause menu](../../../docs/glossary.md#glossary_pause_menu)'s "Back to event list" becomes "Back to RTC 101", and the [drill summary](../../../docs/glossary.md#glossary_drill_summary)'s "Event list" becomes "RTC 101". In authoring builds, the main menu's choice is written to the rep log as `[rep] event=menu menu=main choice=rtc_101|custom_drill|options|data`.

**Blocked by:** None (can start immediately)

**Status:** resolved

- [x] A scenario boots to the main menu, chooses RTC 101 and reaches the event list: the rep log shows `menu=main choice=rtc_101`, then the event list.
- [x] B on the event list returns to the main menu with the cursor on RTC 101 (shot).
- [x] Custom Drill opens the custom drill's settings menu (`choice=custom_drill`).
- [x] Options and Data open their retail menus, and B returns to the main menu on that item.
- [x] Only four items can be selected: moving down from Data, or up from RTC 101, never reaches a removed item (shot of each end).
- [x] The pause menu says "Back to RTC 101" and the summary says "RTC 101" in a curriculum drill (shots), and both still go back to the event list.
- [x] `boot-to-event-list` and the fragments that reach Custom drill go through the new main menu. Every scenario that includes them passes.
- [x] `boot-to-training`, `training-vanilla` and `training-pause-cpu`, and any scenario that only includes them, are deleted.
- [x] `dev.py check` passes, and the matching build is unchanged.

## Comments

- **Selections and frames.** The four items are retail's own main menu selections, so labels, descriptions, preview panels and the light colour all follow retail's per-selection frames and indices: RTC 101 is `SEL_MAIN_1P` (0: "1-P Mode" label frame, description 0x81, 1P preview), Custom Drill is `SEL_MAIN_VS` (1: "VS. Mode" frame, description 0x82, VS preview), Options is `SEL_MAIN_SETTINGS` (3, description 0x84) and Data is `SEL_MAIN_DATA` (4, description 0x85). Trophies (`SEL_MAIN_TOY`, 2, description 0x83) is locked through `mn_80229938`, which is what makes the cursor skip it and the menu lay out the other four with no gap (retail's own mechanism, as for 1-P Mode's hidden item). Tickets 03/04 should replace the 1-P Mode and VS. Mode label frames (and descriptions 0x81, 0x82; preview frames for selections 0 and 1).
- **Hooks.** `src/training/main_menu.c` (`training_main_menu_selectable`, `training_main_menu_confirm`) is called from two one-line hooks in `src/melee/mn/mnmain.c` (`mn_80229938` for `MENU_KIND_MAIN`, and the top of `mn_8022DB10`'s confirm branch). RTC 101 calls `mnEvent_8024E838(0, true)` as 1-P Mode's Event Match did. B on the event list (`mnEvent_8024D864`) goes to `mn_80229894(MENU_KIND_MAIN, SEL_MAIN_1P, 3)` instead of the 1-P submenu; that covers both leaving the list and leaving it after a drill (the menu scene re-enters as `MENU_KIND_EVENT`).
- **Custom Drill for now.** It opens the event list on `training_drill_custom_index()` and sets a one-shot request; `training_custom_settings_input` opens the settings menu on the first frame the list exists (`training_main_menu_take_custom_request`). B on the settings menu still returns to the list, and the Custom drill's pause/summary items keep "Back to event list"/"Event list" (chosen in `back_label()` in `pause.c` and `show_summary_menu()` in `tech_chase.c`, by `training_drill_is_custom`). Ticket 02 replaces all of this.
- **Boot timing.** `boot-to-main-menu` pressed three `start`s; the third was already choosing 1-P Mode on the developer's snapshot (so the old "main menu" at frame 194 was really the 1-P submenu). It now presses two and waits, still ending at frame 194, on the main menu with the cursor on RTC 101. A choice made while the main menu's entry save ("SAVING") runs was not written to the rep log when tried, so scenarios choose after it.
- **Scenarios.** New: `main-menu-rtc-101`, `main-menu-ends`, `main-menu-back-labels`, prefixes `boot-to-custom-drill` and `main-menu-to-custom-drill`. `boot-to-event-list` is boot + A on RTC 101; every `custom-*` scenario now starts from `boot-to-custom-drill` instead of 23 downs on the list. `boot-to-character-select` now reaches the Custom drill's character select (VS. Mode is gone). `event-restart-change-list` needed one fewer down (the pause menu keeps its cursor between pauses, which it hadn't allowed for).
