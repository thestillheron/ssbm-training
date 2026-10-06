# Layout editor and print layout

Status: ready-for-agent

## Problem Statement

Finding good [layouts](../../docs/glossary.md#glossary_layout) by editing coordinates in C, rebuilding and relaunching is slow. Layouts are spatial (where on the stage, how high, how far away the player starts), and the right answer comes from feel: try one, play a few reps, nudge it. I need to place the landing point, drop heights and player start in the game while playing, then get the values out as drill data. Players in custom drills want the same thing, for example to set up a platform tech chase.

## Solution

**Edit layout** in the pause menu opens a layout editor over the frozen match. The real fighters act as markers:
- the opponent hangs at the selected drop height above the landing point
- the player stands at its start

Controls:
- the stick moves the landing point (snapped to floors and platforms)
- the C-stick moves the player start (snapped)
- the d-pad selects and adjusts drop heights, and adds or removes them

Leaving the editor resumes the drill with the edited layout, so it can be tried immediately. In builds with [authoring tools](../../docs/glossary.md#glossary_authoring_tools), **Print layout** writes the edited layout to the log as drill data ready to paste into the curriculum table. Players never see Print layout.

## User Stories

1. As a curriculum designer, I want to edit a drill's layout in the game, so that I can find good positions by feel.
2. As a curriculum designer, I want the opponent shown at the selected drop height above the landing point while editing, so that I can see exactly where it will fall from.
3. As a curriculum designer, I want the player fighter shown at its start while editing, so that I can judge the distance.
4. As a curriculum designer, I want the landing point to move with the stick and snap onto the floor or platform below, so that it's always valid.
5. As a curriculum designer, I want to move the landing point between surfaces (from the ground up onto a platform), so that platform drills are easy to set up.
6. As a curriculum designer, I want the player start to move with the C-stick and snap to the floor below, so that it's valid too.
7. As a curriculum designer, I want to add, remove, select and raise or lower drop heights with the d-pad, so that I can set up mixed timing.
8. As a curriculum designer, I want to see the current values (landing x, heights, player offset) as text while editing, so that I can make precise changes.
9. As a curriculum designer, I want a fine/coarse movement toggle (e.g. hold a trigger for fine), so that I can place things both quickly and precisely.
10. As a curriculum designer, I want leaving the editor to resume the drill with the edited layout immediately, so that I can test a change in a couple of reps.
11. As a curriculum designer, I want edits to apply only for the session, without changing the compiled drill, so that experimenting is safe.
12. As a curriculum designer, I want to revert to the drill's original layout, so that I can throw away a bad edit.
13. As a curriculum designer, in an authoring build, I want Print layout to write the layout as drill-table text to the log, so that I can paste it into the curriculum.
14. As a curriculum designer, I want `dev.py` to show the printed layout text after a session, so that I don't have to dig through Dolphin's log.
15. As a player, I want Edit layout available in custom drills, so that I can set up my own situations (custom-drills).
16. As a player, I want no Print layout option or developer text in my build, so that nothing confusing appears.
17. As the agent, I want the editor reachable and controllable from scenarios, so that I can test it with shots and the log.

## Implementation Decisions

- **Edit mode** is a state of the training module: the match stays frozen (as for menus), the fighters are placed as markers each time the layout changes, and fed opponent input is neutral. Snapping uses drills-as-data's floor ray-casts. A move that would leave no floor below is clamped to the last valid position.
- **Controls (to tune in play-testing):**
  - stick: landing point x (and y across surfaces: up/down steps between surfaces above or below)
  - C-stick: player start offset
  - d-pad up/down: change the selected height
  - d-pad left/right: select height
  - X: add a height (copy of the selected one)
  - Y: remove the selected height
  - Z or a trigger held: fine movement
  - B or Start: leave
- **Range heights:** the editor edits min and max as two selectable heights.
- **Session copy.** The editor edits a working copy of the current drill. Revert restores the compiled drill. Custom drills edit their own working copy.
- **Print layout** is compiled only with authoring tools. It writes a rep log line with a `layout` prefix whose payload is the drill table's layout fields as C initializer text. `dev.py`'s rep log command gains a filter to show only layout lines.
- **On-screen values** are drawn with the shared menu/text component from drill-select.

## Testing Decisions

- **Scenarios:** open Edit layout, move the landing point, add a height, leave → rep log `drill_start` / drop lines show the new layout. Print layout in an authoring build → a `layout` line whose values match. A non-authoring build has no Print layout entry (shot) and no `layout` strings (guard test, as in authoring-tools-flag).
- **Shots** of the editor on Final Destination and on a stage with platforms (landing point on a platform).
- **CLI test:** the layout filter of the rep log command, on a fixture log.

## Out of Scope

- Saving edited layouts to the memory card.
- Editing settings other than the layout (that's custom-drills).
- Free 3D camera or on-stage gizmos beyond the fighters and text.
