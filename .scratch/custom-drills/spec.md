# Custom drills: player-chosen settings and layouts

Status: ready-for-agent

## Problem Statement

The curriculum is a fixed path, but players have their own needs: drilling the one option they keep missing, practising on their main stage, setting up a platform tech chase from a real match, or turning vanish on early. They need to set up a drill themselves, with the same settings the curriculum uses and the same layout editor.

## Solution

A **Custom drill** entry in drill select opens a settings menu covering everything a [drill](../../docs/glossary.md#glossary_drill) has:
- tech option pool (toggle each option)
- drop heights: one, a list, or a range (and the layout editor for the rest of the layout)
- vanish on/off
- length: reps or time, and how many
- pass mark
- player reset on/off
- opponent facing

It works on any stage. The defaults are a sensible layout in the middle of the main floor. Starting it runs the drill like any other, including Edit layout from the pause menu. Settings last for the session.

## User Stories

1. As a player, I want a Custom drill entry in drill select, so that I can set up my own practice.
2. As a player, I want to toggle each tech option in the pool, so that I can practise exactly the options I struggle with.
3. As a player, I want the menu to refuse an empty option pool, so that the drill always has something to do.
4. As a player, I want to choose between a fixed drop height, a few heights, or a range, so that I control the timing variation.
5. As a player, I want to turn vanish on or off, so that I can train the early read whenever I like.
6. As a player, I want to set the length as a number of reps or a time limit, so that I can do quick sets or long sessions.
7. As a player, I want to set a pass mark, or none, so that I can set myself a target.
8. As a player, I want to turn the player reset off, so that I can chase from wherever I end up.
9. As a player, I want to set the opponent's facing, so that I can practise reading techs from either facing.
10. As a player, I want to use the layout editor to place the landing point and my start, including on platforms, so that I can recreate real situations.
11. As a player, I want custom drills to work on any stage I picked, so that I can practise on the stages I play.
12. As a player, I want sensible defaults when I first open Custom drill (middle of the main floor, all options, one height), so that I can start immediately.
13. As a player, I want my custom settings kept while I stay in Training Mode, so that I can restart or come back to them without setting everything up again.
14. As a player, I want the same feedback and summary as curriculum drills, so that custom practice is just as informative.
15. As the agent, I want the custom settings written to the rep log at drill start, so that scenario tests can confirm them.

## Implementation Decisions

- **Settings menu** built with drill-select's menu component. Each item edits a field of a session-scoped custom drill record (the same record type as curriculum drills).
- **Defaults:** landing point at the main floor's middle (found by a floor ray-cast at the stage's centre), one drop height (the curriculum's base height), player start at the curriculum's default offset, all four options, vanish off, 10 reps, no pass mark, reset on, facing the player.
- **Any stage:** the custom drill takes the stage in play. If the stored layout has no floor on a new stage, it falls back to the defaults.
- **Layout editor:** Edit layout in the pause menu works on the custom drill's working copy, and the edits are kept in the session's custom drill. There's no Print layout in player builds (authoring builds keep it).
- **Session only:** no memory card saving (a later item in priorities).

## Testing Decisions

- **Scenarios:** open Custom drill → turn off all options except tech away → set 5 reps → start → the rep log shows `drill_start` with those settings and 5 reps of `option=tech_away` → `drill_end`. Trying to empty the pool is refused (shot). A custom drill on a stage with platforms with the landing point moved onto a platform logs drops onto that surface.
- **Shots** of the settings menu.

## Out of Scope

- Saving custom drills to the memory card.
- Weighted options and an adjustable visible-frame count for vanish.
- Sharing custom drills between players.
