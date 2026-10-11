# Locked camera: a fixed view while vanish is on

Status: resolved

## Problem Statement

[Vanish](../tech-vanish/spec.md) hides the opponent after the first 5 frames of its tech so the player has to read the tech early. The camera still follows the fighters, so its movement (zooming and panning as the opponent rolls) gives the tech option away on reaction. The player can learn to react to the camera instead of the first 5 frames.

## Solution

A [locked camera](../../docs/glossary.md#glossary_locked_camera): whenever the current drill has vanish on, the camera is held on one fixed view per stage for the whole drill event. The view is the framing the normal camera has with both fighters standing on the main floor at the two ledges, so a rep anywhere on the stage stays on screen.

## Implementation Decisions

- **Lock condition.** A drill event is in play and the current drill's `vanish` is on and its stage has a view. Vanish implies the lock; there is no separate setting, and the Custom drill's Vanish row (description "Opponent vanishes, camera stays put") does both. The Custom drill's stage is its stage select pick, so the lock follows it.
- **Stages.** The six tournament-legal stages (Fountain of Dreams, Pokemon Stadium, Yoshi's Story, Dream Land, Battlefield, Final Destination), keyed by external stage id as `Drill::stage`. A stage with no view keeps the normal camera.
- **Views are measured, not computed.** `src/training/locked_camera.c` holds a const table of eye, interest and FOV. The authoring-only pause item Print camera writes the camera as a table row; the `locked-camera-capture-*` scenarios put both fighters at the ledges of each stage (Custom drill, Edit layout, tech in place only) and print it.
- **Applied every frame.** `fn_8002F360`, the camera's per-frame update, calls `training_locked_camera_update` after retail's mode callback in the training build. It writes the view into the camera's transform and its copy (current and target, so no tween) and zeroes the quake translation, so a hit's screen shake can't move it either. The camera mode stays standard, so everything that reads the camera keeps working.
- **Whole drill event.** The lock holds from the match's first frame, through the pause menu, editor and summary, and ends with the match scene; vanilla Training Mode never has it.
- **High drops.** The frame is not made tall enough for every drop height: an opponent dropped from above the frame is out of view until it falls into it, which costs nothing because the reaction window starts at its landing.

## Testing Decisions

- `locked-camera-lock` plays drill 14 (vanish on, Final Destination) with the player off the ledges and prints the camera at two pauses several reps apart: both equal the table's Final Destination row. Without the lock the same positions would give a camera that follows the fighters.
- The capture scenarios double as per-stage checks: each prints the camera it measured at its stage, and `drill_start` shows the stage id they reached.

## Out of Scope

- A view for stages outside the six, a locked camera without vanish, and a camera setting of its own.
- Pokemon Stadium's transformations: the view was measured on the base stage and the transformed stages were not checked.
