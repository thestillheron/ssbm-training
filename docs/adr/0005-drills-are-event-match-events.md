# Drills are Event Match events

Supersedes [ADR-0003](0003-tech-chase-drills-start-inside-training-mode.md).

In the training build, each [drill](../glossary.md#glossary_drill) is an event in the 1P menu's Event Match, as UnclePunch's training pack does it. Picking one goes through Training Mode's player + CPU character select to a match on the drill's own stage, where our own pause menu and drill summary take over. Event Match gives us a main-menu entry, an event list with descriptions, character select and "back to the list" routing without any menu art, and choosing a drill fixes its stage, so there is no "wrong stage" to handle. The hooks are one-line `TRAINING_BUILD` calls in the Event Match mode and the event select screen. Training Mode goes back to vanilla. Our events never write Event Match's own scores or cleared bits, because the training build shares the player's real save.

## Considered Options

- **Stay inside Training Mode with an in-match drill list (ADR-0003):** fewest hooks, but Training Mode's pause menu and CPU settings need suppressing, the list has to handle drills for other stages, and it has no main-menu entry.
- **A new game mode with its own main-menu entry:** cleanest separation, but needs a scene-table edit and a main-menu label (3D models and textures) for little gain over reusing Event Match.
