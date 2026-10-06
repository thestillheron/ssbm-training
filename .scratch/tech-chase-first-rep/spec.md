# Tech-chase first rep: an opponent that drops, techs in place, and repeats

Status: ready-for-agent

## Problem Statement

I want to build a tech-chasing [curriculum](../../docs/glossary.md#glossary_curriculum), but nothing in the training build yet controls an opponent. Before designing drills, menus or feedback, I need to see the basic loop working in the real game: an opponent falling in tumble, landing, teching, then the whole thing resetting so I can try again. Without that loop there's nothing to play-test, and every later feature (outcomes, tech options, vanish, the editor) has nothing to hang off.

## Solution

In vanilla 1P Training Mode (per [ADR-0003](../../docs/adr/0003-tech-chase-drills-start-inside-training-mode.md)), the training build takes over the CPU opponent and runs an endless series of [reps](../../docs/glossary.md#glossary_rep) on Final Destination. In each rep the opponent drops in tumble, from rest, from one fixed point above the middle of the stage. It [techs in place](../../docs/glossary.md#glossary_tech_in_place) on landing because we feed it the L/R press, per [ADR-0004](../../docs/adr/0004-force-tech-options-through-cpu-input.md). The rep ends when it is actionable again. After about a second both fighters are reset and the next rep begins. The player can attack it freely; nothing is scored yet.

A committed scenario boots the training build into this state, so this and every later feature can be checked with `dev.py run --scenario`.

## User Stories

1. As a player, I want the opponent to appear in tumble above the stage when Training Mode starts, so that I'm practising tech chasing straight away.
2. As a player, I want the opponent to fall from rest (no sideways drift), so that each landing happens at the same place.
3. As a player, I want the opponent to land and tech in place every time, so that I can learn one option before facing a mix.
4. As a player, I want the tech to look and sound exactly like a real one (sound, flash, animation, face-up/face-down variant), so that what I learn here transfers to real matches.
5. As a player, I want the opponent to be reset and dropped again about a second after it becomes actionable, so that I can do many reps quickly.
6. As a player, I want to be reset to my start position (grounded, standing, facing the opponent, 0%) at the start of every rep, so that each rep starts the same.
7. As a player, I want the opponent's percent reset to 0 each rep, so that knockback from my punish is consistent.
8. As a player, I want my hits and grabs on the opponent to behave normally, so that I can try out punishes even before scoring exists.
9. As a player, I want the opponent to do nothing else on its own (no AI movement, shielding or attacks), so that it only does what the drill tells it to.
10. As a player, I want the next rep to start cleanly even if I've knocked the opponent away or grabbed it, so that I never have to fetch it myself.
11. As a player, I want this to work whatever characters I picked at character select, so that I can practise my matchup.
12. As the developer, I want the opponent's tech produced by feeding it controller input rather than calling the game's state functions, so that it behaves like a real player's tech (ADR-0004).
13. As the developer, I want the takeover to apply only in the training build and only in Training Mode, so that the matching build stays byte-identical and other modes are untouched.
14. As the developer, I want the upstream changes limited to single guarded hook lines, so that upstream merges stay easy.
15. As the agent, I want a committed scenario that goes from boot to a running rep in Training Mode on Final Destination, so that I can verify this and later features with `run --scenario`.
16. As the agent, I want that scenario to screenshot the opponent in tumble, during its tech, and after the reset, so that I can confirm the loop from images.
17. As the agent, I want the boot-to-Training-Mode part to be its own reusable scenario, so that later features can include it.

## Implementation Decisions

- **Hosting.** Runs inside vanilla Training Mode (ADR-0003). A new training module sets itself up when the Training scene is entered: one guarded hook in the Training scene's on-enter creates a per-frame think object, as vanilla Training already does for its menu. Training Mode's own CPU behaviour settings must not interfere. Suppressing Training Mode's pause menu belongs to drill-select, not here.
- **Opponent input.** The opponent stays a CPU-slot fighter. The game's AI is silenced and its input is replaced every frame by ours, through one guarded hook between the game's CPU tick and the point where CPU input is read into the fighter's input (ADR-0004). Outside a rep the fed input is neutral.
- **Forcing tech in place.** One L (or R) press, timed so that the opponent lands within the game's tech window (a press counts for a limited number of frames, and there's a lockout after a press, so press exactly once per rep), with the stick neutral. The press is timed from a per-frame landing prediction (current height above the floor and fall speed), the same idea the game's own AI uses. Because the fall starts from rest at a known height, a fixed delay from the drop is an acceptable first cut, but prediction is preferred as it carries over to mixed heights.
- **Rep lifecycle** (a small state machine owned by the training module): *reset* (place both fighters, zero velocities, percent 0, opponent faces the player) → *drop* (opponent enters tumble at the drop point, from rest) → *airborne* (feed the tech press at the right frame) → *landed* (tech in progress) → *ended* (opponent actionable again) → ~1 s pause → *reset*. "Actionable" means the opponent has left the knockdown/tech states and is back in a neutral, actionable state.
- **Placement.** Teleporting a fighter means setting its position and then resyncing its collision data and environment collision box, as fighter spawning does. The opponent enters tumble with the game's own tumble-entry function, after its velocities and knockback velocity are cleared. The player is put into its standing state at the start position.
- **Fixed layout for this slice.** A hardcoded [layout](../../docs/glossary.md#glossary_layout): landing point at the middle of Final Destination's main floor, one drop height, player start a fixed distance to one side. Drill data comes in drills-as-data.
- **Scenarios.** A `boot-to-training` scenario (boot → main menu → 1P → Training → pick characters with fixed inputs → Final Destination) and a `tech-chase-first-rep` scenario that includes it and takes shots at tumble, tech and reset.
- **Glossary.** Use rep, layout and tech in place in code comments and docs.

## Testing Decisions

- Good tests check behaviour visible from outside: the game state reached and shown, not internal variables.
- **Seam: `dev.py run --scenario`** with shots (existing). The `tech-chase-first-rep` scenario is the acceptance check. The agent reads the shots and confirms tumble, a tech in place, and a second rep starting.
- `dev.py check` must pass: the matching build stays byte-identical (the guarded hooks compile out) and the training build links.
- No unit tests for game C code, matching the repo today. The rep log (authoring-tools-flag) is added next and gives text-based checks from then on.
- Prior art: the `boot-to-character-select` scenario and the title marker hook.

## Out of Scope

- Detecting success or failure, and scoring (rep-outcome).
- Any tech option other than tech in place (tech-options).
- Drill data, rep counts, drill end (drills-as-data).
- Menus, pausing, and suppressing Training Mode's own menu (drill-select).
- Stages other than Final Destination.

## Further Notes

- Facts gathered from the decomp for this work (function names are still mostly addresses): tumble entry, the landing decision (tech roll check, then tech in place, then missed tech), the tech-window test against the fighter's frames-since-L/R counter and its lockout, and the CPU input fields that the fighter input step reads. The window and lockout lengths come from game data loaded at runtime, not constants.
- The game's CPU command-script queue (which the AI itself uses to press R before landing) is a possible alternative to a hook, if it turns out to be enough on its own.
