# Curriculum v1: the first designed sequence of drills

Status: resolved

## Problem Statement

The features (tech options, mixed heights, vanish, platforms) are only tools. A player who opens the mode needs a designed path that starts easy and builds the tech-chase skill one step at a time. Choosing the drills, their order, their layouts and pass marks is design work, and it needs to be tested in play.

## Solution

The developer designs and play-tests a first [curriculum](../../docs/glossary.md#glossary_curriculum), authoring layouts with the layout editor and pasting printed layouts into the drill table. The agreed skeleton:

1. One height, one option, once for each option: tech in place, tech in, tech away, missed tech. Vanish off.
2. One height, two options, in four drills, in this order: missed tech + tech in place, tech in + missed tech, tech away + tech in place, tech in + tech away. Vanish on.
3. One height, all four options.
4. Two or three heights, one option, then all four.
5. A height range, all four options.
6. Platform drills: opponent lands on a platform, player on the ground or the platform.

[Vanish](../../docs/glossary.md#glossary_vanish) is on in every drill with more than one option (steps 2–6) and off in the one-option drills (step 1, and the one-option drills in step 4). It is introduced as soon as options are mixed, so the player is never tempted to react to which way the opponent is rolling, which is too late. It applies to all four options, including a missed tech, so a missed tech can't be told from the techs by whether it disappears.

### Length and pass mark

A rep takes about 200 frames, about 3.3 s: the drop to landing (~25 frames), landing to actionable (~35 on average over the options) and the fixed 120-frame pause between reps (`PAUSE_FRAMES`), plus ~20 frames of reset. The pause dominates, so the option mix barely changes the length. This is an estimate, not a measurement.

- One-option drills last about 2 minutes: **35 reps**, pass mark **28** (80%).
- Every other drill lasts about 4 minutes: **70 reps**, pass mark **56** (80%).

Final Destination for steps 1–5.

## User Stories

1. As a player, I want the first drills to isolate one tech option at a single timing, so that I can learn each option's look and punish in isolation.
2. As a player, I want the next drills to mix two options, in four pairs, then all four, so that I learn to tell them apart.
3. As a player, I want timing variation introduced only once I can tell the options apart, so that one new difficulty comes at a time.
4. As a player, I want vanish introduced as soon as options are mixed, so that I never get used to reacting to the opponent's movement.
5. As a player, I want platform drills last, so that I first master the basics on flat ground.
6. As a player, I want every drill's pass mark to mean I'm ready for the next, so that progression feels earned.
7. As a player, I want each drill's description to tell me what it's training and what to look for, so that I understand the point of it.
8. As a player, I want one-option drills to take about 2 minutes and the rest about 4, so that the easy drills don't drag and the hard ones give enough reps to show a real rate.
9. As the developer, I want to iterate on layouts in the game and paste the printed result into the table, so that authoring is quick.
10. As the developer, I want to play-test the curriculum across several matchups (including extremes like Bowser vs Sheik tech away), so that problem drills are found.
11. As the developer, I want problem matchups noted as candidates for per-matchup overrides, so that the later override item has real cases.

## Implementation Decisions

- The content is drill table entries, per drills-as-data. The agent's role is to transcribe printed layouts and write descriptions on request. The design calls are the developer's.
- Vanish is a flat 5 visible frames in every curriculum drill, and it now lasts until the opponent is actionable, for all four options. This changes the finished tech-vanish feature, so it is [issue 01](issues/01-vanish-until-actionable-for-all-options.md), the first ticket of this curriculum, and the drills with vanish on depend on it. The amended behaviour is in [tech-vanish/spec.md](../tech-vanish/spec.md).
- Lengths are counts of counted reps (void reps don't count), not time limits, so the rep count per drill is fixed.
- All drills are unlocked. There are no saves.

## Testing Decisions

- Play-testing by the developer. The rep counts rest on an estimated rep length: measure the real length from the rep log during play-testing, and if it is more than ~15% off 200 frames, rescale the counts.
- The agent can run a scenario that starts every drill once and checks each `drill_start` in the rep log for errors (e.g. an invalid layout).

## Out of Scope

- Getup options after a missed tech (a possible "Part 2" curriculum).
- Per-matchup overrides (recorded as findings here, built later).
- The tech in place + tech in and tech away + missed tech pairs (left out on purpose).
