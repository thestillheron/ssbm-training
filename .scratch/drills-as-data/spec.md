# Drills as data: layouts, option pools, length and pass marks

Status: ready-for-agent

## Problem Statement

So far the rep loop runs on hardcoded values: one place, one height, endless reps. The curriculum needs many different [drills](../../docs/glossary.md#glossary_drill), each with its own [layout](../../docs/glossary.md#glossary_layout), option pool, timing variation, length and pass mark, and it needs a drill to finish so I can see whether I passed. I want a drill to be a piece of data that I can write, review and change without touching the rep logic.

## Solution

A drill is a data record, compiled into the training build as a table of curriculum drills. The rep loop runs whichever drill is current. A drill specifies:
- its stage
- its layout: a landing point snapped to a floor or platform, a set of drop heights (a list, or a min–max range), and the player's start as an offset from the landing point, snapped to the floor below
- the opponent's facing
- whether the player is reset each rep
- the tech option pool
- whether the opponent vanishes
- its length (a number of reps or a time limit)
- its pass mark

When the drill ends, a summary shows the score and pass/fail.

## User Stories

1. As a curriculum designer, I want to define a drill as a data record, so that I can add and adjust drills without changing rep logic.
2. As a curriculum designer, I want to set the landing point anywhere on a floor or platform, snapped onto its surface, so that drills can be on the main floor or a platform.
3. As a curriculum designer, I want to give one drop height, so that every rep has the same timing.
4. As a curriculum designer, I want to give a list of drop heights, picked at random each rep, so that timing varies between a few known values.
5. As a curriculum designer, I want to give a min–max height range, picked uniformly each rep, so that timing varies continuously.
6. As a curriculum designer, I want to set the player's start as an offset from the landing point, snapped to the floor below that spot, so that "opponent on the platform, player on the ground below" works.
7. As a curriculum designer, I want the opponent to face the player by default, with an override, so that facing can become part of the read.
8. As a curriculum designer, I want to turn off the player reset for a drill, so that advanced drills can chase from wherever the player ended up.
9. As a curriculum designer, I want to set the tech option pool per drill, so that drills step up from one option to all four.
10. As a curriculum designer, I want a vanish on/off setting per drill, so that vanishing can be introduced in later drills.
11. As a curriculum designer, I want to set a drill's length as a number of counted reps or a time limit, so that both styles are possible.
12. As a curriculum designer, I want to set a pass mark per drill, so that each drill has a clear target.
13. As a curriculum designer, I want each drill to name its stage, so that layouts are only used where they make sense.
14. As a curriculum designer, I want each drill to have a short name and a one-line description, so that it can be listed in menus.
15. As a player, I want the drill to end after its reps or time, so that a session has a clear finish.
16. As a player, I want a summary at the end showing my score, the pass mark, and whether I passed, so that I know if I'm ready to move on.
17. As a player, I want the summary to show my success rate per tech option, so that I know which option I struggle with.
18. As a player, I want void reps not to count towards a drill's length, so that every counted rep is a real attempt.
19. As a player, I want a time-limited drill to let the rep in progress finish when time runs out, so that the last attempt isn't cut off.
20. As the agent, I want the drill name and its settings written to the rep log at drill start, so that tests know which drill produced which reps.
21. As the agent, I want an invalid layout (no floor below the landing point or player start) reported clearly in the rep log and on screen, not as a crash, so that bad data is easy to spot.
22. As the developer, I want a temporary way to choose the current drill (e.g. the first table entry, switched with a debug input in authoring builds) until drill-select exists, so that drills can be tried straight away.

## Implementation Decisions

- **Drill record** (a C struct; the table is a const array in the training sources, per the session decision that the curriculum lives as C tables):
  - stage id
  - name, description
  - landing point (x, plus a y hint used to choose which surface to snap to)
  - heights: kind (list or range) plus values, in game units above the snapped landing surface
  - player start offset (dx, dy hint)
  - opponent facing: toward player (default), away, left, right
  - reset player (default yes)
  - option pool (a set of the four tech options)
  - vanish (bool)
  - length kind (reps or time) plus value
  - pass mark (number of successes)
- **Snapping.** At drill start and at each reset, ray-cast down from the stored point with the stage collision library's floor check to find the surface below: the landing surface, and the player's floor. A missed ray-cast makes the drill invalid. Both report and refuse to run. Platform floors are allowed.
- **Stage mismatch.** A curriculum drill runs only on its named stage. Until the drills have a mode of their own (which will pick the stage itself), drill-select offers only drills for the stage in play and says which stage the others need.
- **Rep loop** reads all settings from the current drill. tech-options' pool and constants move here.
- **Drill end and summary.** Count counted reps (or time). When the length is reached and the current rep finishes, stop dropping and draw a summary as text: score, pass mark, pass/fail, and per-option successes. From the summary the player can restart the drill. Leaving it is drill-select's job, and until then restart is the only action.
- **Rep log** gets `drill_start` (name plus settings) and `drill_end` (score, passed) lines.

## Testing Decisions

- **Rep log seam:** a test drill with one height produces identical `landing` timing across reps. With a three-height list, every height appears. With a range, all heights fall within it. A rep-count drill ends after exactly N counted reps (voids excluded). The pass/fail line is correct for a scripted no-input run (0 successes → fail).
- **Shots:** the summary screen. A platform layout on a stage with platforms (opponent lands on the platform).
- Invalid layout: a deliberately bad test drill (authoring builds only) logs an invalid-layout error.
- Prior art: the earlier rep log scenarios.

## Out of Scope

- The real curriculum content (curriculum-v1).
- Menus for choosing drills (drill-select) and custom settings (custom-drills).
- Saving anything to the memory card.
- Weights or non-uniform selection.
- Per-matchup overrides (a later item in priorities).
