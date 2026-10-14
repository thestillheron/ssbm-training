# 05: Screen headings

**What to build:** The heading strip at the top of the screen reads "RTC 101" on the event list and "Custom Drill" on the Custom Drill settings screen, using `heading_rtc_101.png` and `heading_custom_drill.png` from `assets/main-menu` ([spec](../spec.md)). They're 168×28 images, encoded to I4 and patched with ticket 03's patcher into both `MnMaAll` language variants. They show in the top-level heading slot, with nothing in the second-level slot where "Event Match" was. Retail's own screens (Options, Data and their submenus) keep their headings.

**Blocked by:** 02, 03

**Status:** resolved

- [x] The event list shows "RTC 101" with nothing beneath it (shot).
- [x] The Custom Drill settings screen shows "Custom Drill" with nothing beneath it (shot).
- [x] Options and Data screens show their retail headings (shot of one).
- [x] The same headings show after switching to Japanese (shot).

## Comments

- **How the heading is chosen.** The event list and the Custom Drill screen are the same retail scene (`cur_menu` 7), so the panel (`MenMainPanel_Top`) shows the same frames on both. Its top-level joint 84 reads image-table entry 0 ("1-P Mode") there and its second-level joint 85 entry 1 ("Event Match"). Entries 0 and 1 of joint 84 ("1-P Mode", "VS. Mode") are shown only on 1-P Mode's and VS. Mode's own screens (menus 1, 2, 6-14, 16-18, 33), which the training build cannot reach, so they are patched to "RTC 101" and "Custom Drill". Entry 1 of joint 85 is used by the event scene alone and is patched to a blank image (I4 intensity 0 is fully transparent), so the second level is empty. Options, Data and their submenus use entries 3 and 4 of joint 84 and other entries of joint 85, all retail's. `screen_headings()` in `tools/archive_patches.py`, descriptors `.usd` `0x563f8`, `0x56410`, `0x564b4`; `.dat` `0x56338`, `0x56350`, `0x563f4`.
- **Run-time part.** `training_main_menu_heading(custom_drill)` (main_menu.c) points the image table's entry 0 at entry 1 on the Custom Drill screen and back on the list. It runs when the event scene opens from the main menu (`open_event_list`) and from `training_event_list_show` (also on return from a drill, which rebuilds the scene). A freshly loaded archive always has entries 0 and 1 different, so the saved retail pointer can never be stale. No material or TEV changes.
- **Look.** The heading is drawn small at the strip's left, as retail's submenu headings are. Shots `shots/05/heading-*` in the session scratchpad: list, Custom Drill, list again (swap undone), Options and Data (retail), and the same after switching to Japanese (`heading-ja-*`; Options keeps its Japanese heading).
- Regression runs: `custom-screen-routes`, `main-menu-art`, `event-list-scroll`. The panel's other retail text (the "SELECT CHARACTER" outline, Japanese "SELECT") is untouched.
