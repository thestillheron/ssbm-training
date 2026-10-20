# Custom drill: crash after an edited layout and a new opponent

Status: ready-for-agent

## Problem Statement

A player sets up a [custom drill](../../docs/glossary.md#glossary_custom_drill), plays it, tunes it with [Edit layout](../../docs/glossary.md#glossary_edit_layout), and then goes back to the character select to try the same layout against a different opponent. The game crashes as the next match loads, so the player loses the layout they just tuned and has to start the game again.

Reproduction:

1. Open Custom Drill from the main menu and Start.
2. Pick Marth for the player and Falco for the opponent.
3. Pick Final Destination on the stage select.
4. Once the drill is running, open the [pause menu](../../docs/glossary.md#glossary_pause_menu) and choose Edit layout.
5. Raise or lower Falco's drop height slightly, then leave the editor (the drill restarts with the edited layout).
6. Choose Change characters in the pause menu.
7. Change the opponent from Falco to Jigglypuff.
8. On the stage select, pick Final Destination.

Expected: the match opens on Final Destination and the custom drill starts, Marth against Jigglypuff, with the edited layout.

Actual: the game crashes.

## Solution

Changing characters between custom drill matches always loads the next match, whichever fighter changed (player, opponent or both), whether the layout was edited or not, and whichever stage is picked next (the same one or another). The edited layout in the custom drill's [session copy](../../docs/glossary.md#glossary_session_copy) carries over to the new match as it does today. If the new opponent's reach leaves the layout with no floor for the player, the drill shows its [invalid layout](../../docs/glossary.md#glossary_invalid_layout) notice as usual, and does not crash.

## User Stories

1. As a player, I want to change the opponent after editing a custom drill's layout and still get into the match, so that I can try my layout against another character.
2. As a player, I want to change my own character after editing a custom drill's layout and still get into the match, so that I can practise the same setup with a second main.
3. As a player, I want to change both characters at once after editing the layout, so that I can switch to a whole new matchup without restarting the game.
4. As a player, I want my edited layout (landing point, drop heights, player start) kept after the character change, so that I don't have to tune it again.
5. As a player, I want to pick the same stage again after changing characters, so that I keep practising on my main stage.
6. As a player, I want to pick a different stage after changing characters, so that I can try the matchup on another stage.
7. As a player, I want a layout that does not fit the new characters to show the invalid layout notice rather than crash, so that I know to edit it.
8. As a player, I want to go back to the character select several times in a row in one session, so that I can cycle through opponents.
9. As a player, I want backing out of the stage select (B) to the character select and then picking again to load the match, so that changing my mind is safe.
10. As a player, I want changing characters with an unedited custom drill to keep working, so that the fix does not trade one crash for another.
11. As a player, I want changing characters on a [curriculum](../../docs/glossary.md#glossary_curriculum) drill (with or without an edited layout) to keep working, so that RTC 101 is not affected.
12. As a player, I want picking the same characters again after Change characters to keep working, so that the case fixed before does not come back.
13. As a player, I want Restart drill and Retry to keep working after a character change, so that the rest of the pause menu is unaffected.
14. As a player, I want the next character select after a changed-character match to load normally, so that I can keep changing characters.
15. As a developer, I want a regression scenario for the reported steps, so that the crash cannot come back unnoticed.
16. As a developer, I want a control scenario with the same steps minus the layout edit, so that I know whether the edit is part of the cause.
17. As a developer, I want the rep log to say which fighters a drill started with, so that a scenario can check the character change happened without reading a shot.

## Implementation Decisions

- **Diagnose first.** The root cause is not yet known. The edit itself does not reload anything: leaving the editor with a changed layout restarts the drill in the running match (the tech chase's restart), with no scene change. So the first task is to reproduce with the regression scenario and the no-edit control, and read the Dolphin log at the crash, before choosing a fix. Use the `diagnosing-bugs` skill.
- **Where to look.** The prior fix for "crash after Change characters" (Event Match's preload in the training event module) reloads the fighter files when the picks differ from the last match's, following retail's single-player order (gmclassic.c). That fix was checked only on a curriculum drill (no stage select) with a changed *player*. This report differs in three ways, any of which may be the cause:
  - the custom drill goes through the stage select between the character select and the match, which loads its own files;
  - the *opponent* changes, not the player, so the opponent's previous fighter files must be released and the new one added (the prior fix re-adds the player and waits for it, but may not release or reload the opponent's slot correctly);
  - the layout was edited, so the custom drill's session copy holds non-default values (a landing y and drop height moved by the editor) that the next match fits and resolves against the new opponent's reach.
- **Fix in the training event module's match load** if the cause is the loader, keeping retail's order of cache operations, and keeping the "same fighters need none of it" case unchanged (doing the reload for the same fighters crashed the next character select). If the cause is the layout (for example a resolve or teleport with values the new opponent's reach makes invalid), fix it in the layout or tech chase modules so an unusable layout ends in the invalid layout notice.
- **Everything stays behind `TRAINING_BUILD`**, in the training code, as the developer workflow describes. The matching build is untouched.
- **Rep log:** the `drill_start` line gains the player's and the opponent's character kinds (authoring build only), so scenarios can assert the fighters in play.

## Testing Decisions

- A good test drives the game the way a player does and checks only what the player and the rep log can see: the match loads, the drill starts, the fighters and layout are the ones expected. It does not inspect loader internals.
- One seam: [scenarios](../../docs/glossary.md#glossary_scenario) played as movies with `dev.py run --scenario`, checked through the [rep log](../../docs/glossary.md#glossary_rep_log) and shots.
- New scenarios:
  - **Regression:** the reported steps from `boot-to-custom-drill` (Marth against Falco on Final Destination, Edit layout, change the drop height, leave, Change characters, Jigglypuff for the opponent, Final Destination). Passes when the log shows a second `drill_start` with stage Final Destination, the edited height and Jigglypuff as the opponent, and a shot shows the match running.
  - **Control:** the same steps without the layout edit. Run it before the fix to learn whether the edit matters; keep it as a regression afterwards.
  - Worth adding if the cause turns out to be general: a changed player on the custom drill, and a different stage after the change.
- Existing scenarios that must still pass: `event-change-characters` (changed player, then the same picks again, on a curriculum drill), `custom-drill-edit-layout` (edits kept across restart and re-entry), `custom-drill-to-stage-select`, `custom-drill-new-stage`, `custom-drill-defaults`.
- Prior art: `event-change-characters` (the previous character-change crash, with its log signature `psInitDataBanks: unknown version`), `custom-drill-edit-layout` (pause menu navigation to Edit layout and back out to the character select and stage select on the custom drill).
- Run `dev.py check` before merging.

## Out of Scope

- Changing how Change characters, the stage select or the layout editor look or behave beyond not crashing.
- Saving custom drill layouts to the memory card, or keeping them per character.
- Refitting or resetting the edited layout when the characters change: it carries over as it does now.
- Crashes on other routes (event list, main menu) unless the diagnosis shows they share this cause.

## Further Notes

- The previous fix's commit message and code comment record that the same fighters must not get the reload, and that the reload follows gmclassic.c. Any change to the load order should keep both.
- Scenario frame numbers assume fast disc and the snapshot save with every character and stage unlocked (developer workflow, Scenarios). Jigglypuff's and Marth's positions on the character select need measuring, as Luigi's were for `event-change-characters`.
- `dev.py run` needs the user's focus permission, and `dev.py stop` afterwards.
