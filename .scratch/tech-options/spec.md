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
  - **Missed tech:** no L/R press while airborne, and no A/B, so the game doesn't go straight from bounce into a getup attack. After landing, wait a fixed down time, then feed a plain getup input (stick up). The down time is a drill-agnostic constant for now, shorter than the game's own auto-getup timer. **Decided: `MISSED_TECH_DOWN_TIME` = 45 frames (0.75 s), counted from the landing frame**, defined with the pool in `tech_options.h`. Measured: the game's auto-getup takes over 270 frames, and with 45 the opponent is lying down visibly (shots) and is actionable 76 frames after landing (45 plus a 31-frame getup). The stick-up input is held from then until it stands.
- **Selection.** At rep start, choose uniformly from the pool using the game's random number function. The pool is a set of options, defaulting to tech in place only until drills-as-data supplies it.
- **Verification in the log.** The rep log gets `option` (chosen) and `result_state` (what the game entered: tech in place, roll forward/back resolved to in/away, or knockdown), plus `forced_ok`.
- **Ordering.** Option choice happens before the drop, so later features (vanish, feedback) can read it.
- **One rule for "toward".** The stick that forces a roll and the check of the roll that came out (`forced_ok`, `player_side`) both use `side_of_player` in `tech_chase.c`: the sign of (player x - opponent x), and at equal x the opponent's facing. They cannot disagree at equal x. Not exercised live: equal x is a measure-zero case a scenario can't reliably hit, so it is checked by the shared helper, not by a rep.
- **Stick direction is re-read each frame, not latched.** The spec's "held across the landing" is implemented as a full stick held from the drop until the landing whose side follows the player each frame. Latching the side when the press is decided was tried live (tech in, player dashing right past the opponent): 3 of 14 reps came out away because the player crossed the opponent between the press and the landing (`forced_ok=false`). The game reads the stick on the landing frame, so where the player stands then is what counts. Flipping the stick in the air is not a flick problem because the stick history is written as already held (below). `forced_ok` stays honest: it compares the result with the player's side at the landing, so a crossing in the last frame or two still shows as `false`.
- **Result is a separate enum from option.** The log's `result_state` is `in-place|in|away|knockdown|wall|walljump|ceil`. Any non-tech landing is `knockdown`, which satisfies `forced_ok` only for the missed tech option, so a failed press that ends in a knockdown is flagged for the other three.
- **Deliberate deviations from the sketch above** (kept, each justified):
  - `self_vel.x` is zeroed each frame of the fall while a roll is being forced. A fully held stick adds air drift, which carried the opponent across the player before it landed and changed the side. Drift during the fall is out of scope, so it is removed rather than modelled.
  - The stick history (`fp->input.lstick[0].x` and `active_timer.lstick.x`) is written directly alongside `cpu.lstick`. A stick that starts or flips during the tumble is read as a flick, which cancels the tech; writing it as already held is what a player holding the stick into the hit would produce.
  - The opponent's input hook acts only on frames the rep loop ran (`rep_loop_ran`, set by the loop and cleared by the hook), not on a scene check: the scene index is the mode's state index, not the scene kind, so it cannot scope the hook to Training Mode.
  - `--tech-pool` on `dev.py build`/`run` compiles a different `TECH_OPTION_POOL` for `tech_chase.c` only, so scenarios for each option need no header edit. The header's pool stays the default.
  - `player_side` (`left|right`, where the player stands at landing) is an extra authoring-only log field, so the scenarios can assert that both sides were covered.

## Testing Decisions

- **Rep log seam:** a pool of one option yields that option on every rep with `forced_ok`, for each of the four options, with the player on the left and on the right of the landing point. A pool of all four produces every option over ~40 reps (a loose check, not a distribution test).
- Shots of each option mid-animation confirm visually that tech in moves toward the player and tech away moves away.
- Prior art: the rep-outcome scenarios.

## Out of Scope

- Getup choices after a missed tech (getup attack, getup roll, staying down). That's a later item in priorities.
- Weights, anti-streak rules or shuffled bags (decided: uniform for now).
- Wall and ceiling techs.
- Drift or horizontal velocity during the fall.
