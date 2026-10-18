# Character drop heights

Status: resolved

## Problem Statement

Every [drill](../../docs/glossary.md#glossary_drill) drops the opponent from the same [drop height](../../docs/glossary.md#glossary_drop_height), 40, whatever its character. That suits fast fallers. For slow fallers, the wait before each landing is frustratingly long. From 40, Falco lands in 22 frames and Fox in 20, but Jigglypuff takes 41 and Samus 39. That wait repeats on every [rep](../../docs/glossary.md#glossary_rep) of a 35- or 70-rep drill, and it sets no harder test of reaction: it is just dead time.

## Solution

Each opponent character gets its own default drop height, its [character drop height](../../docs/glossary.md#glossary_character_drop_height), from a per-character table.

The table's starting values come from max fall speed, on a straight line from Jigglypuff to Falco:
- Jigglypuff (1.3) drops from 21.
- Falco (3.1) drops from 40, today's height.
- Every other character sits in between in proportion to its max fall speed.

The developer will tune the values by hand from there. With these values, the time from drop to landing runs from 19 to 27 frames across the cast, where it is now 20 to 41.

A drill's drop heights are now one of two kinds:
- **Character**: the opponent drops from its table height. This is the default for every [curriculum](../../docs/glossary.md#glossary_curriculum) drill and for the [custom drill](../../docs/glossary.md#glossary_custom_drill).
- **Explicit**: a fixed height, a list or a range, used exactly as set and never scaled. Heights become explicit when someone sets them, either in the layout editor or in the custom drill's settings menu.

When the opponent's character changes, explicit heights go back to Character, so a new opponent always starts at its own height.

## User Stories

1. As a player practising against Jigglypuff, I want it to drop from a lower height than Falco, so that I'm not waiting about 40 frames for every landing.
2. As a player, I want each opponent character's drop to take roughly the same time, so that the rhythm of a drill doesn't depend on which character I'm chasing.
3. As a player, I want fast fallers such as Falco to keep today's drop height of 40, so that drills I already know against them feel the same.
4. As a player on any RTC 101 drill, I want the drop height to follow the opponent without setting anything, so that the improvement is automatic.
5. As a player starting a custom drill with default settings, I want the opponent to drop from its character drop height, so that the custom drill's default matches the curriculum's.
6. As a player, I want only the opponent's character to affect the drop height, so that changing my own character changes nothing about the drop.
7. As a player using Edit layout on a drill with Character heights, I want the opponent marker to hang at the height that opponent will actually drop from, so that what I see is what I get.
8. As a player in the layout editor, I want my first raise or lower of a drop height to start from the character's height, so that I'm adjusting from what I was seeing, not from 40.
9. As a player who changes a drop height in the layout editor, I want that exact height used once I finish editing, so that I can set the height I want directly.
10. As a player who only moves the landing point or player start in the layout editor, I want the drop height to keep following the character, so that an unrelated edit doesn't freeze the height.
11. As a player who opens the layout editor and closes it without changing anything, I want the drill to stay on character drop heights.
12. As a player who changes the drop heights row in the custom drill's settings menu, I want those heights used exactly as set, so that a chosen list or range means what it says.
13. As a player who changes the opponent's character after editing a height, I want the drop height to go back to the new opponent's character drop height, so that a height tuned for one character doesn't carry over to another.
14. As a player who changes the opponent after setting a list or range in the custom drill's settings menu, I want the heights to go back to the new opponent's character drop height, so that "changing characters resets the drop height" holds everywhere.
15. As a player who goes back through character select and picks the same opponent again, I want my edited heights kept, so that re-confirming characters doesn't throw away my work.
16. As a player who changes only my own character, I want my edited heights kept.
17. As a player using Next drill, I want the next curriculum drill to start on character drop heights, so that each drill starts from its own defaults.
18. As a player using Revert in the layout editor on a curriculum drill, I want the heights to go back to following the character, not to a fixed number.
19. As a player using Revert in the layout editor on the custom drill, I want it to keep the heights as it does today, so that Revert still only resets the landing point and player start there.
20. As a player whose RTC 101 drill shows "edited layout" today, I want a drill on character drop heights to count as unedited, so that its history isn't marked edited for no reason.
21. As a player with existing drill history, I want my saved attempts kept and graphed alongside new ones, so that the change doesn't cost me my progress record.
22. As the developer, I want the character drop heights in a readable table with one row per character, so that I can tune any character at my leisure.
23. As a reader of the code, I want the table's comment to explain where the starting values came from (each character's max fall speed and the line from Jigglypuff to Falco), so that the reason for each value is clear.
24. As the developer, I want any character not in the table to fall back to 40, so that an unusual opponent still gets a sensible drop.
25. As the developer, I want Print layout on a Character drill to print a Character marker rather than one opponent's current height, so that pasting a printed layout into the drill table never quietly fixes one character's height into a curriculum drill.
26. As the developer, I want Print layout on an edited drill to print the explicit heights exactly, so that a layout I tried is the layout I paste.
27. As the developer reading the rep log, I want each drop to log the opponent's character and the height it actually dropped from, so that I can see the table working.
28. As the developer reading the rep log, I want the drill start to show that its heights are Character, so that a scenario can tell Character and explicit apart.
29. As the developer, I want the authoring test drills with deliberate lists and ranges (30/60/90, 30–90, no heights) to keep those explicit heights, so that their tests still test lists, ranges and invalid heights.
30. As the developer, I want a check to fail when the table is missing a playable character, so that a character can't silently fall back to 40.
31. As a player choosing Zelda or Sheik, I want each to use its own row, since they fall at different speeds.
32. As a player chasing Ice Climbers, I want the drop to follow Popo's row, so that the pair drops consistently.

## Implementation Decisions

- **New height kind.** The drill's height kind gains a Character kind alongside List and Range. The drill table writes it as a kind with no meaningful height values. Every curriculum drill moves from a list of one 40 to Character, as does the custom drill's default (which replaces the custom base height of 40). The authoring test drills that use a single 40 move to Character too. The ones built around explicit heights (three heights, height range, no heights) stay explicit.
- **Character drop height table.** A new training module holds one row per character the opponent can be, keyed by fighter kind. It has a single lookup: the character drop height for a fighter kind, falling back to 40 for any kind without a row. The per-fighter-kind punish-window store is prior art for keying by fighter kind.
- **Table rows.** There are 26 rows. Zelda and Sheik each have their own row. Ice Climbers is Popo's row. The table is a hand-written table of numbers, not a formula evaluated at runtime, because the developer wants to tune values one at a time and wants the table to show the idea plainly. A comment above it gives the derivation: height = 21 + (max fall speed − 1.3) × 19 / 1.8, with each character's max fall speed beside its row, and it says the values have been tuned by hand since.
- **Starting values,** rounded to tenths, from the max fall speeds in each fighter's data on the disc:

  | Height | Characters (max fall speed) |
  |---|---|
  | 40.0 | Falco (3.1) |
  | 37.9 | Captain Falcon (2.9) |
  | 36.8 | Fox (2.8) |
  | 32.6 | Donkey Kong, Roy (2.4) |
  | 30.5 | Marth (2.2) |
  | 29.8 | Link, Young Link, Sheik (2.13) |
  | 28.4 | Ganondorf (2.0) |
  | 27.6 | Yoshi (1.93) |
  | 27.3 | Bowser, Pikachu, Pichu (1.9) |
  | 26.6 | Ness (1.83) |
  | 25.2 | Mario, Dr. Mario, Mr. Game & Watch (1.7) |
  | 24.2 | Kirby, Luigi, Ice Climbers (1.6) |
  | 23.1 | Peach, Mewtwo (1.5) |
  | 22.1 | Samus, Zelda (1.4) |
  | 21.0 | Jigglypuff (1.3) |

- **Picking a rep's height.** A drill on Character drops the opponent from the table height for the opponent's current fighter kind. List and Range behave as they do now. Nothing is ever scaled.
- **Layout editor.** On a Character drill, the opponent marker hangs at the opponent's table height, and the values text shows that height. The first raise or lower of the height makes the session copy's heights explicit: a list of one, starting from the table value, then adjusted as usual. Moving the landing point or the player start leaves the heights on Character, and so does closing the editor without a height change. This applies to curriculum drills and to the custom drill.
- **Revert.** On a curriculum drill, Revert puts the heights back to Character, because it restores the table entry. On the custom drill, Revert keeps the heights, as it does today.
- **Same layout and edited layout.** Two Character drills have the same heights. A curriculum drill on Character counts as unedited, which is what history's edited-layout marker reads.
- **Print layout.** For a Character drill, Print layout writes the Character kind with no heights, in a form that pastes back into the drill table unchanged. For an explicit drill, it prints the kind and values as now.
- **Custom drill settings menu.** Any change to the drop heights row (its value, or switching between Fixed, List and Range) makes the custom drill's heights explicit. The question of how the row shows a Character default is deferred (see Out of Scope). Until it is decided, the row shows and steps from a Character default exactly as it does today from a fixed 40.
- **Reset on opponent change.** The training code remembers the opponent's fighter kind from the last drill start. When a drill starts with a different opponent kind, the current drill's heights go back to Character. That is the curriculum drill's session copy, or the custom drill's record, whether the heights were set in the layout editor or the settings menu. The same opponent re-picked, or a change of the player's character alone, keeps them. Next drill already loads the next drill's own heights.
- **Rep log.** The drill_start line's height kind reads `character` for Character drills. Each drop line adds the opponent's fighter kind and keeps the height it actually used, logged as an integer as now. This replaces the earlier idea of logging a `base=` height, which has no meaning now that nothing is scaled. The rep-log parser and its test learn the new kind and field.
- **History.** It is unchanged. Saved attempts made at 40 stay in the history and on the graph alongside new ones.
- **Glossary.** The glossary now defines drop height (Character or explicit) and character drop height, and lists "fall height" as a term to avoid. Use those terms in code comments.

## Testing Decisions

- The seam is the **rep log**, checked by acceptance scenarios run through dev.py. These test external behaviour, meaning the height an opponent actually dropped from and the drill's height kind, not how heights are stored. The existing drop-height scenarios (`drill-heights`, `drill-heights-range`, `custom-drop-heights`, `layout-edit-heights`) are the prior art, and `event-change-characters` is the prior art for going back through character select.
- New or updated scenarios:
  - A curriculum drill against the character select's default CPU character logs `height_kind=character`, and each drop is at that character's table height.
  - On a curriculum drill, a height raised in the layout editor sticks across reps. After Change characters to a different opponent, the drop is back at the new opponent's table height.
  - On the custom drill, a list set in the settings menu is used as set. After the opponent changes, the drop is back at the table height.
  - Changing characters with the same opponent re-picked keeps the edited height.
  - Moving only the landing point in the layout editor leaves the drill on Character.
  - The test drills with three heights and a height range still log their explicit heights. Scenarios that today expect `height=40` from the default CPU are updated to that character's table height.
- A static check, modelled on the drill ID check, fails when the character drop height table lacks a row for any playable character. It doesn't check the values, because the developer will tune them.
- The rep-log parser's test covers the new height kind and the opponent field on drop lines.
- Run `dev.py check` before merging.

## Out of Scope

- **How the custom drill's settings menu labels or explains a Character default** (deferred). Players can set an exact height in the layout editor instead.
- Adjusting for gravity or timing the fall, for example choosing heights so that every character takes the same number of frames to land. Max fall speed alone sets the starting values.
- Scaling explicit heights by character.
- Resetting or marking drill history made before this change.
- Opponents outside the 26 playable characters (Master Hand, Crazy Hand, Giga Bowser, Sandbag, the wireframes), beyond the fallback to 40.

## Further Notes

- The fall speeds came from each fighter's common attributes in the disc's fighter data files: gravity, max fall speed (terminal velocity) and fast fall speed. Falco has the highest max fall speed, at 3.1. Captain Falcon is second at 2.9, though he ties Falco on fast fall speed at 3.5.
- Fall times from a drop at rest (each character's gravity, capped at its max fall speed): from 40 they run from 20 frames (Fox) to 41 (Jigglypuff). From the starting table values they run from 19 (Fox) to 27 (Marth, Luigi). The developer may tune the table using these.
