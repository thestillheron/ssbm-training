# Drill select and in-drill controls (inside Training Mode)

Status: ready-for-agent

## Problem Statement

There's no way to choose a drill, restart one or quit, and Training Mode's own pause menu and CPU settings still get in the way. To use the curriculum as a curriculum, I need to see the list of drills, pick one, see its description and target, pause mid-drill, restart it, and go back to the list, the way the UnclePunch training pack lets you browse its events.

## Solution

When Training Mode starts, an in-match drill-select menu appears, drawn as text over the paused game. It lists the [curriculum](../../docs/glossary.md#glossary_curriculum) drills in order (only those for the stage in play; the others say which stage they need), with a description, pass mark and the punish window length for the chosen opponent. Picking one starts it. During a drill, Start opens a pause menu: Resume, Restart drill, Drill select, Quit to character select. Training Mode's own pause menu and CPU settings are suppressed. This stays inside Training Mode, per ADR-0003.

## User Stories

1. As a player, I want a drill list to appear when I enter Training Mode, so that I can choose what to practise.
2. As a player, I want the drills listed in curriculum order with numbers, so that I know the intended progression.
3. As a player, I want each drill's description and pass mark shown when I highlight it, so that I know what it asks of me.
4. As a player, I want to see the punish window length for my chosen opponent on the highlighted drill, so that I understand why some matchups are harder.
5. As a player, I want drills for other stages listed but marked with the stage they need, so that I know to change stage and that they exist.
6. As a player, I want to navigate the menu with the d-pad or stick and confirm with A, back with B, so that it works like Melee's menus.
7. As a player, I want the game paused behind the menu, so that nothing happens while I choose.
8. As a player, I want Start during a drill to open a pause menu, so that I can stop at any time.
9. As a player, I want Resume to continue the drill exactly where it was, so that pausing costs nothing.
10. As a player, I want Restart drill to reset the score and start again from rep 1, so that I can retry a drill cleanly.
11. As a player, I want Drill select to go back to the list, so that I can switch drills without leaving Training Mode.
12. As a player, I want Quit to character select to leave Training Mode as normal, so that I can change characters or stage.
13. As a player, I want the end-of-drill summary to offer Restart, Next drill and Drill select, so that moving through the curriculum is quick.
14. As a player, I want Training Mode's own menu and CPU options not to appear, so that the two systems don't conflict.
15. As a player, I want all curriculum drills available (none locked), so that I can practise whatever I like.
16. As a player, I want a "Custom drill" entry at the end of the list, so that I can find custom mode in the same place (custom-drills fills it in; until then it's hidden or disabled).
17. As the agent, I want menu navigation reachable with scripted input in a scenario, so that I can test every menu path with shots.
18. As the agent, I want menu selections written to the rep log, so that scenario tests can confirm which drill started.
19. As the developer, I want the menu built from the drill table, so that adding a drill needs no menu changes.

## Implementation Decisions

- **Menu framework:** text-based, drawn with the game's text library, built according to the UnclePunch research findings (layout, cursor, colours). A small reusable menu component (items, cursor, confirm/back callbacks) that the pause menu, summary and custom-drills also use.
- **Pausing:** while a menu is up, freeze the match (preferably with the game's own pause mechanism, without its pause UI) and stop feeding opponent input.
- **Suppressing Training Mode UI:** our module takes over Start in Training Mode. Training Mode's menu think and its CPU behaviour settings are prevented from running or affecting the opponent, through the smallest guarded hook possible.
- **Drill list:** generated from the drill table. Drills for other stages are shown as unavailable, with the needed stage's name. The punish window length shown for the opponent comes from a short measurement or a cached value from earlier reps. If neither is cheap, show "—" until it's been measured once in this session.
- **Navigation:** standard Melee conventions (d-pad/stick to move, A confirm, B back, Start opens or closes pause).
- **No locking or saving** in this spec.
- **Rep log** gets `menu` lines for selection events.

## Testing Decisions

- **Scenarios with shots:** boot to Training → the drill list is shown → choose drill 2 → pause → restart → drill select → choose drill 1 → pause → quit to character select. The rep log confirms the drill names at each `drill_start`.
- Training Mode's own pause menu doesn't appear on Start (shot).
- Prior art: the boot-to-training scenario.

## Out of Scope

- A game mode of its own and a main-menu entry (a later item in priorities).
- Locking, progress and memory card saves.
- Edit layout from the pause menu (layout-editor adds it).
- Grouping drills into lessons. The list is flat for now, and grouping can come later if the list gets long.
