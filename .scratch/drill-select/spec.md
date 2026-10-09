# Drill select and in-drill controls (drills as Event Match events)

Status: ready-for-agent

## Problem Statement

There's no way to choose a drill, restart one or quit, and Training Mode's own pause menu and CPU settings still get in the way. To use the curriculum as a curriculum, I need to see the list of drills, pick one, see its description and target, pause mid-drill, restart it, and go back to the list, the way the UnclePunch training pack lets you browse its events.

## Solution

Drills are entered the way UnclePunch's events are: through the 1P menu's **Event Match**. In the training build the Event Match list shows the [curriculum](../../docs/glossary.md#glossary_curriculum) drills instead of the vanilla events, one event per [drill](../../docs/glossary.md#glossary_drill), in curriculum order, with the drill's description and pass mark in the event's description box. Choosing one opens the player + CPU character select (Training Mode's CSS type), and the match starts on the drill's own stage with the drill running. During a drill, Start opens our own pause menu: Resume, Restart drill, Change characters, Back to event list. L+R+A+Start goes straight to character select. The end-of-drill summary offers Restart, Next drill and Event list.

Training Mode goes back to being vanilla: no drills, no drill chooser. This supersedes ADR-0003; see [ADR-0005](../../docs/adr/0005-drills-are-event-match-events.md).

## User Stories

1. As a player, I want the drills listed in Event Match, so that I can choose what to practise from the 1P menu.
2. As a player, I want the drills listed in curriculum order with numbers, so that I know the intended progression.
3. As a player, I want each drill's description and pass mark shown when I highlight it, so that I know what it asks of me.
4. As a player, I want to see the punish window length for my opponent in the pause menu and at drill start, so that I understand why some matchups are harder.
5. As a player, I want each drill to start on its own stage, so that I never have to pick a stage that suits it.
6. As a player, I want to pick my character and the opponent after choosing a drill, so that I can practise against the characters I meet.
7. As a player, I want to navigate the list and menus with the d-pad or stick, A to confirm and B to back out, so that it works like Melee's menus.
8. As a player, I want Start during a drill to freeze the game and open a pause menu, so that I can stop at any time.
9. As a player, I want Resume to continue the drill exactly where it was, so that pausing costs nothing.
10. As a player, I want Restart drill to reset the score and start again from rep 1, so that I can retry a drill cleanly.
11. As a player, I want Change characters to take me to character select for the same drill, so that I can switch matchup without finding the drill again.
12. As a player, I want L+R+A+Start to take me to character select too, as in vanilla matches, so that the habit still works.
13. As a player, I want Back to event list to return to the list with the drill I played highlighted, so that I can pick another.
14. As a player, I want the end-of-drill summary to offer Restart, Next drill and Event list, so that moving through the curriculum is quick.
15. As a player, I want Next drill to keep my characters and move to the next drill's stage, so that I can go straight on.
16. As a player, I want Training Mode to behave exactly as in vanilla Melee, so that it is still there for free practice.
17. As a player, I want the game's own pause menu not to appear during a drill, so that the two menus don't conflict.
18. As a player, I want all curriculum drills available (none locked), so that I can practise whatever I like.
19. As a player, I want playing drills not to touch my vanilla Event Match scores or cleared events, so that my real save is unchanged.
20. As a player, I want a "Custom drill" event at the end of the list (custom-drills adds it), so that I can find custom mode in the same place.
21. As the agent, I want every menu path reachable with scripted input in a scenario, so that I can test them with shots.
22. As the agent, I want menu selections written to the rep log, so that scenario tests can confirm which drill started.
23. As the developer, I want the event list built from the drill table, so that adding a drill needs no menu changes.

## Implementation Decisions

- **Mode entry:** take over `GM_EVENT` in the training build the way the UnclePunch findings describe ("Mode entry, pause, restart and return to menu"), with one-line `TRAINING_BUILD` hooks in the Event Match game mode and the event select screen calling into `src/training/`, not address patches. CSS type 0x17 (player + CPU). Match setup is built from the drill (stage, the two fighters from the CSS) instead of the event data file. Our training module starts in that match and nowhere else.
- **Event list:** the retail event select screen, with its rows, names, descriptions and paging limited to the drill table (test drills only in authoring builds). The vanilla events are not shown in the training build. Text through the game's text library, converted by our shared helper.
- **No vanilla save writes:** our events never write Event Match's high scores or cleared bits. The training build runs as GALE01 and shares the player's real save.
- **Pause menu:** `rules.disable_pausing` so the retail pause never runs, and our Start opens a text menu over a match frozen with the game's own pause flag, sound and HUD calls (findings, "Pause"). Conventions from the findings: any human port's Start opens, the pausing port drives, B backs out without unpausing, only Start closes.
- **Menu component:** a small reusable text menu (items with label, description, enabled flag and confirm callback; cursor; 9-row scrolling window skipping and greying disabled rows; A/B/Start with sounds 2/1/0 via `lbAudioAx_80024030`). The pause menu and the summary use it, and later layout-editor and custom-drills. Highlight by text colour first.
- **Restart drill:** in place (resets the rep loop to rep 1 with a clean score and clock), not a scene reload.
- **Change characters / L+R+A+Start:** end the match and go back to the CSS for the same event. **Back to event list:** end the match and reopen the list on the drill just played (retail's "reopen on the last event" routing).
- **Next drill:** the next drill in the table as the next event, with the same fighters, reloading the match scene (the stage may change). After the last drill, Next drill is disabled.
- **Punish window display:** from a cached measurement of earlier reps against this opponent in the session, or "—" until one rep has measured it.
- **Training Mode:** the drill hook, CPU takeover and drill chooser are removed from Training Mode, which goes back to vanilla behaviour.
- **No locking or saving** in this spec.
- **Rep log** gets `menu` lines for list and menu selections.

## Testing Decisions

- **Scenarios with shots:** boot → 1P → Event Match → the drill list is shown → choose drill 2 → CSS → drill starts → pause → restart → Back to event list → choose drill 1 → pause → Change characters → CSS → start → L+R+A+Start → CSS. The rep log confirms the drill names at each `drill_start`.
- The retail pause UI doesn't appear on Start (shot).
- Training Mode runs no drill and logs no rep events (scenario).
- Every existing scenario that boots to Training to run a drill moves to Event Match entry.
- Prior art: the boot-to-training scenario.

## Out of Scope

- A new main-menu entry or label of our own (Event Match's label is reused).
- Locking, progress and memory card saves.
- Edit layout from the pause menu (layout-editor adds it).
- The Custom drill event itself (custom-drills adds it, with a stage select step).
- Grouping drills into lessons or pages. The list is flat for now.
