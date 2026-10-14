# Main menu: RTC 101, Custom Drill, Options, Data

Status: ready-for-agent

## Problem Statement

Getting to the training means going through vanilla Melee's menus: 1P Mode, then Event Match, then a list whose last row is Custom drill. The way in says "1-P Mode" and "Event Match", not tech chasing, and it passes VS Mode, Trophies and Training Mode, none of which the training build is for. Players load the training build to train. They keep a vanilla disc to play Melee. The menus should make that obvious and look like they were designed for it rather than patched over.

## Solution

The training build's main menu has four items: **RTC 101**, **Custom Drill**, **Options** and **Data** ([ADR-0007](../../docs/adr/0007-training-build-owns-the-main-menu.md)). [RTC 101](../../docs/glossary.md#glossary_rtc_101) is the basic [curriculum](../../docs/glossary.md#glossary_curriculum) of [reaction tech chase](../../docs/glossary.md#glossary_reaction_tech_chase) drills, and opens its [event list](../../docs/glossary.md#glossary_event_list). Custom Drill opens the [custom drill](../../docs/glossary.md#glossary_custom_drill)'s settings menu directly. Options and Data are retail, unchanged. Every other vanilla mode is unreachable.

The new items look like retail ones: their labels, the heading strip at the top of each screen and the right-hand preview panel are art in the retail menu's own texture formats, tinted and highlighted by the menu as retail labels are. The art comes from a Claude Design session first. If that doesn't give a look we're happy with, labels rendered from an open-licence font. As a last resort, our own SIS text drawn over hidden retail labels.

## User Stories

1. As a player, I want the main menu to show only RTC 101, Custom Drill, Options and Data, so that it's clear this disc is for tech chase training.
2. As a player, I want the four items together in that order with no gap, so that the menu looks designed rather than cut down.
3. As a player, I want the cursor to start on RTC 101, so that the curriculum is one press away after boot.
4. As a player, I want RTC 101 and Custom Drill labels in the same style as retail's labels, so that the menu looks like part of the game.
5. As a player, I want our labels to turn black and the cursor to highlight them when hovered, as Options and Data do, so that the menu behaves consistently.
6. As a player, I want a description in the box at the bottom for RTC 101 ("Learn to tech chase on reaction, one step at a time.") and for Custom Drill ("Set up a drill of your own: options, stage, layout, vanish."), so that I know what each item does.
7. As a player, I want the right-hand preview panel to show artwork for RTC 101 and Custom Drill, or nothing, not a list of vanilla modes, so that what I see matches what I'll get.
8. As a player, I want A on RTC 101 to open the list of RTC 101 drills, so that I can start the curriculum.
9. As a player, I want the RTC 101 list to show only the curriculum's drills, with no Custom drill row, so that the curriculum is one clear path.
10. As a player, I want the RTC 101 screen's heading to say "RTC 101", with nothing beneath it, so that I know where I am.
11. As a player, I want B on the RTC 101 list to take me back to the main menu on RTC 101, so that backing out lands where I came from.
12. As a player, I want the pause menu's back item in an RTC 101 drill to say "Back to RTC 101", so that it names where it goes.
13. As a player, I want the drill summary's back item to say "RTC 101", so that it names where it goes.
14. As a player, I want the drill history graph (X on a drill) and everything else on the RTC 101 list to work as it does now, so that nothing I rely on is lost.
15. As a player, I want A on Custom Drill to open its settings menu directly, so that setting up my own drill is one step from boot.
16. As a player, I want the Custom Drill settings menu on the menu background with a "Custom Drill" heading and no drill rows behind it, so that custom practice feels separate from the curriculum.
17. As a player, I want B on the Custom Drill settings menu to take me back to the main menu on Custom Drill, so that backing out lands where I came from.
18. As a player, I want Start on the settings menu to go to character select, stage select and the match as it does now, so that a custom drill plays exactly as before.
19. As a player, I want B on the Custom Drill character select to take me back to its settings menu, so that I can change a setting before playing.
20. As a player, I want the pause menu's back item in a custom drill to say "Settings" and return to its settings menu, so that I can tweak and rerun.
21. As a player, I want the custom drill's summary to offer Restart and Settings, with no Next drill, so that I'm never offered a drill that doesn't exist.
22. As a player, I want my custom settings kept for the session however I leave and return to Custom Drill, so that I don't set them up again.
23. As a player, I want Options and Data to work exactly as they do in vanilla, so that I can still set rumble, sound and screen, and see records.
24. As a player, I want switching the language to Japanese in Options to keep the same main menu, labels and headings, so that the retail 1P/VS menu can't come back.
25. As a player, I want no way to reach 1P Mode, VS Mode, Trophies or Training Mode, accepting I'll use a vanilla disc for those.
26. As a player, I want the main menu to be the same in every build, so that what I play is what was tested.
27. As the developer, I want the patched menu archives written as fresh copies into the assembled game folder, never into the extracted disc, so that `orig/` stays a clean retail extraction.
28. As the developer, I want the archive patcher to change only the textures and strings it targets, so that nothing else in the menu can break.
29. As the developer, I want the label artwork to come from images committed to the repo and converted at build time, so that changing the look means changing an image.
30. As the developer, I want a prototype that shows candidate artwork in the real main menu before the real tool is written, so that we choose the look by eye in the game, not in a design tool.
31. As the developer, I want a fallback to labels rendered from an open-licence font, and finally our SIS text over hidden label models, so that the feature ships whatever happens with the art.
32. As the developer, I want the prototype to show Options and Data both retail and redrawn in our style beside our labels, so that I can decide whether a mixed menu jars.
33. As the agent, I want the main menu's choice written to the rep log, so that scenarios can confirm the routing.
34. As the agent, I want the scenarios that reach the event list or Custom drill to go through the new main menu, so that every existing check keeps working.
35. As the agent, I want the vanilla Training Mode scenarios removed, so that the suite only checks what players can reach.

## Implementation Decisions

**Routing (training build only, behind `TRAINING_BUILD` hooks in the retail menu code)**

- The main menu's top level is retail's menu with four selections: RTC 101, Custom Drill, Options and Data. 1P Mode, VS Mode and Trophies are gone, not hidden: the cursor can't land on them, and the four items are laid out without a gap.
- RTC 101 enters Event Match's event list as 1P Mode → Event Match does now (ADR-0005 stands: drills are still Event Match events). Leaving the list, or returning to it from a drill and then backing out, goes to the main menu with the cursor on RTC 101, not to the 1P submenu.
- The event list holds only the [drill table](../../docs/glossary.md#glossary_drill_table)'s curriculum entries. The Custom drill row and its index past the last drill go.
- Custom Drill enters the custom drill's settings menu as a screen of its own: the menu background, the "Custom Drill" heading, no event list. Whether this reuses the event list scene with the list hidden or is a separate menu state is the implementer's choice, as long as the player sees the screen above. Its Start, character select, stage select and match flow are as now. B on its character select returns to the settings menu. B on the settings menu returns to the main menu on Custom Drill.
- Pause menu: "Back to event list" becomes "Back to RTC 101" in a curriculum drill and "Settings" in a custom drill, which returns to the settings menu.
- Drill summary: "Event list" becomes "RTC 101" in a curriculum drill. In a custom drill the menu is Restart and Settings, with no Next drill.
- Same menu in every build, authoring builds included. No button combination brings back the retail menu.

**Art: what's replaced** (findings in `ref/main-menu/`, see Further Notes)

The main menu archive is `MnMaAll`: `.usd` (English) and `.dat` (Japanese), chosen by the saved language. Both get the same English art, so Options' language switch can't bring retail back.
- **Labels:** each top-level label is its own 176×30 IA4 texture: white fill, black outline, shape in alpha. They are the frames of one texture animation on the cursor model's label joint. The menu's material animation tints the fill orange, or black when hovered. Replace "1-P Mode" with RTC 101 and "VS. Mode" with Custom Drill, or whichever frames the four selections map to once the top level is reordered. Options and Data keep theirs unless the prototype decides otherwise.
- **Headings:** the heading strip at the top of each screen (`MenMainPanel`) is 168×28 I4, in its own italic style: a top-level image and a second-level image. On the RTC 101 list and the Custom Drill settings screen, the top-level image is "RTC 101" or "Custom Drill" and the second level shows nothing.
- **Preview panel:** 128×104 I4, a per-item image on the right of the main menu (`MenMainConTop`). It's designed artwork for RTC 101 and Custom Drill, or blank. It is never a list of RTC 101's drills, which would go stale as the curriculum changes.
- **Descriptions:** SIS strings in `SdMenu` (`.usd` and `.dat`), the main menu's description entries for our two items. These are text, not art.
- All art stays in retail's sizes and formats: two tones, 16 levels, tinted by the menu's existing materials. No TEV or material changes.

**Art: where it comes from, in order**

1. Artwork from a Claude Design session, delivered as images at the exact sizes above, committed to the repo, and converted to IA4 or I4 at build time.
2. If that look doesn't work out, labels and headings rendered at build time from an SIL Open Font License font committed to the repo, styled like retail (white fill, black outline).
3. As a last resort, no archive patching: our SIS text drawn over hidden retail label models.

A throwaway prototype comes first: candidate art patched into the real archive, and shots of the real main menu with each item highlighted and not, with Options and Data both retail and redrawn. The look, and whether Options and Data are redrawn, is chosen from those shots. The real build step is written after that.

**Archive patcher**

- A build step run by `dev.py build`, alongside the existing assembly of the game folder. (As built: it runs in `assemble_game`, i.e. during `dev.py run`, not in `build`; `dev.py check` also applies every patch to a throwaway copy so a `PatchError` surfaces there first.) It writes patched copies of `MnMaAll.usd/.dat` and `SdMenu.usd/.dat` into the game folder as fresh files. `boot.bin`'s game ID patch is the precedent. The extracted disc is read, never written.
- Its interface: the source archive, plus a list of replacements (a texture located by its image descriptor, with an image to put there, or an SIS string entry with its text), producing the patched archive. A replacement image must match the target's size, and each texture replacement also declares the format and size it expects, which the descriptor must match, and a string must fit. Otherwise it's an error, never silently resized or truncated.
- Pixel data is overwritten in place, so offsets, relocations and every other byte stay as they were.

**Rep log (authoring builds)**

- New line for the main menu choice, in the style of the existing menu lines: `[rep] event=menu menu=main choice=rtc_101|custom_drill|options|data`.

## Testing Decisions

Good tests check what the player can see or what the rep log says, never internal state. Three seams:

- **Scenarios with rep log lines (the main seam).** Played as movies against the memory card snapshot, like every existing scenario. New and updated scenarios:
  - Boot to the main menu and choose RTC 101: the `menu=main choice=rtc_101` line, then the event list.
  - B on the RTC 101 list goes back to the main menu.
  - Choose Custom Drill, then Start: the settings menu, then character select, stage select and `drill_start` with the custom settings.
  - B on the Custom Drill settings menu goes back to the main menu.
  - A custom drill's pause "Settings" and summary "Settings" return to the settings menu, and its summary has no Next drill (`event=menu` lines from the shared menu component).
  - A curriculum drill's pause "Back to RTC 101" and summary "RTC 101" return to the event list.
  - Rewrite `boot-to-event-list` and the `custom-drill-*` / `custom-settings-*` fragments to go through the new main menu. They currently scroll to the last event list row. All scenarios that include them must still pass.
  - Delete `boot-to-training`, `training-vanilla` and `training-pause-cpu`, and any scenario that only includes them.
  - Prior art: `event-list-choose-second`, `custom-drill-to-stage-select`, `drill-summary-menu`, `event-restart-change-list`.
- **Shots, checked by eye.** The main menu with each of our two items highlighted and not, the RTC 101 list's heading, the Custom Drill settings screen, and the main menu after switching Options to Japanese. Prior art: the layout editor's and the summary's shots.
- **The archive patcher, in `tools/dev_tests`.** Patch a texture and a string in a copy of a real archive, then check:
  - decoding the texture gives back the input image;
  - the string reads back;
  - every other byte is unchanged;
  - a wrong-sized image or an over-long string is an error;
  - `orig/` is never written.
  
  Prior art: `test_build.py`, `test_memcard.py`.

## Out of Scope

- A release step (ISO or patch against the player's disc). This is "Patch-based release" in priorities. This feature only makes the assembled game folder complete.
- Restyling our own text menus (pause menu, drill summary, settings menu, rep result).
- A second curriculum, such as one for reacting to missed-tech getup options. The main menu would gain an item for it later.
- Any way to reach vanilla modes from the training build.
- Changing the menu's materials or TEV for full-colour art.

## Further Notes

- **Art delivered.** The Claude Design session produced all eight images to this brief, in `assets/main-menu`: four labels (Options and Data redrawn too), two headings and two preview panels. Tickets 04 and 05 put them in the game, and 04's shots replace the separate prototype.
- **Design brief for Claude Design.** Our art is monochrome in game: the fill is white, the menu tints it orange (#cc9933), and it turns black with a #ffcc00 highlight when hovered. The outline stays black. Deliverables are images at exact sizes:
  - a 176×30 label each for "RTC 101" and "Custom Drill";
  - a 168×28 heading each, in the heading strip's italic style;
  - a 128×104 preview panel each, or none.
  
  Each has 16 levels of intensity and alpha. The art should be our own lettering, not traced from Nintendo's textures.
- **References.** `ref/main-menu/` (gitignored, so it isn't committed) holds the retail art decoded from `MnMaAll.usd`:
  - `png_usd/`: labels, heading sheets and preview panels;
  - Python scripts that parse HSD archives, decode GX textures and dump joint trees and material animations: `hsd.py`, `dump.py`, `png.py`, `sheet.py`.
  
  Key facts:
  - the label textures are frames of the texture animation on joint 3 of `MenMainCursor_Top`;
  - the main menu's description indices are 0x81–0x85;
  - the event menu's breadcrumb uses panel joints 84 (top level) and 85 (second level).
- It hasn't been checked in game which `MnMaAll` variant an NTSC-U disc loads by default (`.usd` is assumed for English). Patching both makes this moot, but the prototype should confirm what it's looking at.
- Supersedes the "Game mode of its own" entry under Later in priorities.
