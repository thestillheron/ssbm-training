# Tech options: in place, in, away and missed tech, chosen at random

Status: ready-for-agent

## Problem Statement

An opponent that always techs in place only teaches one reaction. Tech chasing is about reading which [tech option](../../docs/glossary.md#glossary_tech_option) the opponent picked and covering it. I need the opponent to be able to do all of them, and to pick randomly from a set so that I have to react instead of anticipate. Players think of rolls relative to themselves ("toward me", "away from me"), not relative to the opponent's facing, so the options need to be expressed that way.

## Solution

The opponent can carry out any of the four tech options: [tech in place](../../docs/glossary.md#glossary_tech_in_place), [tech in](../../docs/glossary.md#glossary_tech_in) (roll toward the player), [tech away](../../docs/glossary.md#glossary_tech_away) (roll away from the player) and [missed tech](../../docs/glossary.md#glossary_missed_tech). After a missed tech it lies down for a fixed time, then gets up normally. Each rep it picks uniformly at random from an option pool. All of it is driven by fed controller input (ADR-0004).

## User Stories

1. As a player, I want the opponent to tech in, rolling toward me, so that I can practise covering a roll into me.
2. As a player, I want the opponent to tech away, rolling away from me, so that I can practise chasing a roll away.
3. As a player, I want tech in and tech away to mean toward and away from me whichever side I'm on and whichever way the opponent faces, so that the option names always match what I see.
4. As a player, I want the opponent to miss the tech and lie down, so that I can practise punishing a missed tech.
5. As a player, I want a missed tech to be followed, after a fixed time, by a plain getup, so that the missed-tech window is consistent while I learn it.
6. As a player, I want the opponent to pick its option at random from the pool each rep, so that I have to react, not guess a pattern.
7. As a player, I want each option in the pool equally likely, so that practice is balanced.
8. As a player, I want a pool of one option to always produce that option, so that early drills can isolate one option.
9. As a player, I want every option to look and sound exactly as it does against a human, so that recognition transfers to real matches.
10. As a player, I want a hit on a downed opponent (e.g. a jab reset) to behave as the game normally does, so that missed-tech punishes are realistic.
11. As the agent, I want the chosen option written to the rep log each rep, so that I can confirm the selection and its distribution.
12. As the agent, I want the option the game actually carried out to be logged as well as the one chosen, and a mismatch flagged, so that a failed forcing (e.g. a mistimed press) is caught.
13. As the developer, I want the pool and the missed-tech down time set in one place, so that drills-as-data can supply them.
14. As the developer, I want the random choice to come from the game's own random source, so that it behaves like the rest of the game.

## Implementation Decisions

- **Inputs per option** (all fed into the opponent's input, per ADR-0004):
  - **Tech in place:** one L/R press timed into the tech window, stick neutral.
  - **Tech in / tech away:** the same press, with the stick held fully left or right across the landing. The direction is in world terms: toward the player is the sign of (player x − opponent x), and away is the opposite. The game turns this into its own forward/back roll from facing, so the drill doesn't need to know facing. If the two x positions are equal, use the opponent's facing as "toward".
  - **Missed tech:** no L/R press while airborne, and no A/B, so the game doesn't go straight from bounce into a getup attack. After landing, wait a fixed down time, then feed a plain getup input (stick up). The down time is a drill-agnostic constant for now, shorter than the game's own auto-getup timer. Pick a value that matches common play and record it here.
- **Selection.** At rep start, choose uniformly from the pool using the game's random number function. The pool is a set of options, defaulting to tech in place only until drills-as-data supplies it.
- **Verification in the log.** The rep log gets `option` (chosen) and `result_state` (what the game entered: tech in place, roll forward/back resolved to in/away, or knockdown), plus `forced_ok`.
- **Ordering.** Option choice happens before the drop, so later features (vanish, feedback) can read it.

## Testing Decisions

- **Rep log seam:** a pool of one option yields that option on every rep with `forced_ok`, for each of the four options, with the player on the left and on the right of the landing point. A pool of all four produces every option over ~40 reps (a loose check, not a distribution test).
- Shots of each option mid-animation confirm visually that tech in moves toward the player and tech away moves away.
- Prior art: the rep-outcome scenarios.

## Out of Scope

- Getup choices after a missed tech (getup attack, getup roll, staying down). That's a later item in priorities.
- Weights, anti-streak rules or shuffled bags (decided: uniform for now).
- Wall and ceiling techs.
- Drift or horizontal velocity during the fall.
