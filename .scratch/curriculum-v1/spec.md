# Curriculum v1: the first designed sequence of drills

Status: ready-for-human

## Problem Statement

The features (tech options, mixed heights, vanish, platforms) are only tools. A player who opens the mode needs a designed path that starts easy and builds the tech-chase skill one step at a time. Choosing the drills, their order, their layouts and pass marks is design work, and it needs to be tested in play.

## Solution

The developer designs and play-tests a first [curriculum](../../docs/glossary.md#glossary_curriculum), authoring layouts with the layout editor and pasting printed layouts into the drill table. The agreed skeleton:

1. One height, one option, once for each option: tech in place, tech in, tech away, missed tech.
2. One height, two options (e.g. in place vs away, in vs away).
3. One height, all four options.
4. Two or three heights, one option, then all four.
5. A height range, all four options.
6. Steps 3–5 again with vanish on.
7. Platform drills: opponent lands on a platform, player on the ground or the platform.

Defaults: 10 counted reps, pass mark 8, Final Destination for steps 1–6.

## User Stories

1. As a player, I want the first drills to isolate one tech option at a single timing, so that I can learn each option's look and punish in isolation.
2. As a player, I want the next drills to mix two options, then all four, so that I learn to tell them apart.
3. As a player, I want timing variation introduced only once I can tell the options apart, so that one new difficulty comes at a time.
4. As a player, I want vanish introduced after mixed timing, so that the early read is forced only once my reactions are solid.
5. As a player, I want platform drills last, so that I first master the basics on flat ground.
6. As a player, I want every drill's pass mark to mean I'm ready for the next, so that progression feels earned.
7. As a player, I want each drill's description to tell me what it's training and what to look for, so that I understand the point of it.
8. As the developer, I want to iterate on layouts in the game and paste the printed result into the table, so that authoring is quick.
9. As the developer, I want to play-test the curriculum across several matchups (including extremes like Bowser vs Sheik tech away), so that problem drills are found.
10. As the developer, I want problem matchups noted as candidates for per-matchup overrides, so that the later override item has real cases.

## Implementation Decisions

- The content is drill table entries, per drills-as-data. The agent's role is to transcribe printed layouts and write descriptions on request. The design calls are the developer's.
- Vanish is a flat 7 visible frames in every curriculum drill.
- All drills are unlocked. There are no saves.

## Testing Decisions

- Play-testing by the developer.
- The agent can run a scenario that starts every drill once and checks each `drill_start` in the rep log for errors (e.g. an invalid layout).

## Out of Scope

- Getup options after a missed tech (a possible "Part 2" curriculum).
- Per-matchup overrides (recorded as findings here, built later).
