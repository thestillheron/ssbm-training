# Scenario movies: frame-exact scenario playback through Dolphin movies

Status: resolved

## Problem Statement

Scenarios are meant to take the training build from boot to a known game state the same way every time, but they don't. Live playback turns each step's frame count into milliseconds and sends keystrokes to the focused Dolphin window on a wall clock. Dolphin reads the keyboard once per emulated input poll, and emulation doesn't run at exactly real time. So a hold of N frames lands on N-1, N or N+1 polls depending on phase and emulation speed. On the character select and stage select screens the cursors accelerate the longer the stick is held, so one frame either way moves the cursor onto a different character or stage. `boot-to-training` sometimes picks the wrong fighter or misses Final Destination, and nothing downstream of it (the tech-chase drill scenarios) can be trusted until it's checked by eye.

Tuning can't fix this. Longer holds and generous waits make dropped presses rare, but they can't make a cursor move exact, and splitting a move into short taps multiplies the error. The emulator-piloting spec already named Dolphin movie (`.dtm`) playback as "the expected route if determinism becomes necessary". It's necessary now.

## Solution

Keep scenario files as the single source of truth and compile each one into a Dolphin movie. A movie stores the GameCube pad state for every input poll, and Dolphin injects that state at the poll itself, so playback is frame-exact by construction. `run --scenario <name>` boots the freshly built training build with the movie generated from the scenario, from power-on, so the same build and the same scenario always reach the same state.

Scenario authoring doesn't change: the same plain-text step grammar, `include`, `shot` and comments, counted in frames. Frames now mean emulated frames exactly, not "roughly 16.7 ms each". Input comes from the movie, not the keyboard, so stray keystrokes or focus changes can't corrupt the input. When the movie ends, Dolphin hands control back to the keyboard-bound controller, so live `pad` piloting carries on from the reached state as it does today.

Movies are generated build output, never committed and never started from a savestate. A savestate restores the whole of RAM, including the old build's code, so a savestate movie would silently run the build it was recorded with, not the one just built.

## User Stories

1. As the agent, I want `run --scenario <name>` to reach the same game state on every run, so that I can trust the state a scenario reports without checking it by eye.
2. As the agent, I want character select cursor moves in a scenario to land on the same character every time, so that drills always start with the intended fighter.
3. As the agent, I want stage select cursor moves to land on the same stage every time, so that drills always start on Final Destination.
4. As the agent, I want a step's `:N` hold to mean exactly N emulated frames of input, so that frame counts I work out once stay correct.
5. As the agent, I want `wait:N` to mean exactly N emulated frames with neutral input, so that menu timing is reproducible.
6. As the agent, I want to keep writing scenarios in the existing plain-text grammar, so that nothing I or the developer already know or wrote has to change.
7. As the agent, I want `include` to keep working in movie playback, so that shared prefixes such as `boot-to-main-menu` stay written once.
8. As the agent, I want combined controls such as `stick-up+stick-left` and `stick-down+b` to come out as the matching combined pad state, so that diagonals and button-plus-direction inputs behave as they did live.
9. As the agent, I want every existing control (`a b x y z start l r`, main stick, C-stick and D-pad directions) supported in movies, so that every existing scenario can be compiled.
10. As the agent, I want `scenario <name> --movie <path>` to write the movie generated from a scenario without touching Dolphin, so that I can generate and inspect a movie anywhere, including with no Dolphin and no keyboard bindings.
11. As the agent, I want movie generation to fail with the same clear errors as dry run for a malformed step, unknown control, missing include or include cycle, so that a broken scenario fails before anything is built or launched.
12. As the agent, I want movie generation not to depend on my Dolphin keyboard bindings, so that a rebind or an unbound control can't break a movie.
13. As the agent, I want `run --scenario <name>` to generate the movie, build, then launch Dolphin on the training build with that movie in one command, so that going from a code change to a verified state stays one step.
14. As the agent, I want the movie to start from power-on rather than a savestate, so that what plays is always the build I just made.
15. As the agent, I want movie playback to use the memory card already in Dolphin (the save with everything unlocked), so that character select and stage select layouts match the ones the scenarios were written for.
16. As the agent, I want `shot` steps to still produce labelled screenshots at the printed paths during a movie run, so that I can check the checkpoints a scenario defines.
17. As the agent, I want `run --scenario` to return once the movie has finished and its shots are taken, so that I know when the state is reached and live piloting can take over.
18. As the agent, I want the keyboard-bound controller to take over when the movie ends, so that I can pilot live from the scenario's end state with `pad`.
19. As the agent, I want `scenario <name>` against an already-running Dolphin to keep its existing live keyboard behaviour, so that I can still play a short scenario mid-session where a power-on movie can't apply.
20. As the agent, I want a way to force the old live keyboard path for `run --scenario`, so that I can compare the two or fall back if movie playback misbehaves.
21. As the developer, I want movie playback to ignore my keyboard and mouse while the movie runs, so that touching my machine during a long boot can't derail it.
22. As the developer, I want generated movies kept under the gitignored build output and not committed, so that there's never a stale binary script out of step with its scenario.
23. As the developer, I want the scenario file to stay the reviewable artifact, so that changes to piloting show up as readable diffs.
24. As the developer, I want `dry-run` to keep printing the timed plan, so that I can still read what a scenario does before running it.
25. As the developer, I want `boot-to-training` retuned for frame-exact playback (hold lengths checked under exact frames, shorter safety padding), so that the scenario I rely on most is both reliable and faster.
26. As the developer, I want the hand-recorded `boot-to-training.dtm` and its savestate removed from the scenarios folder, so that nobody mistakes a savestate movie for a supported route.
27. As the developer, I want the developer workflow doc to explain movie playback, power-on only, the memory card assumption and the savestate trap, so that future agents don't reintroduce savestate movies.
28. As the developer, I want "movie" defined in the glossary next to "scenario", so that the two terms aren't confused: the scenario is the script, the movie is what it compiles to.
29. As the developer, I want the opt-in end-to-end test to cover movie playback, so that the live path is verified when I choose without taking focus during routine checks.
30. As the developer, I want a clear error if the Dolphin I have configured can't play the generated movie (for example it rejects the header), so that a version mismatch is reported, not mistaken for a scenario bug.
31. As the developer, I want the focus-permission rule to still apply to `run --scenario`, so that the agent still asks before launching Dolphin even though the movie input itself needs no focus.

## Implementation Decisions

- **Scenarios stay the source of truth; movies are compiled output.** A new pure-logic module turns an expanded scenario plan into a Dolphin movie (DTM bytes). It sits next to the existing scenario and pad modules and, like them, has no Win32 and no Dolphin dependency. It takes the frame-indexed list of steps that the scenario module already produces (after `include` expansion) and doesn't go through the millisecond key-event plan.
- **Frame model.** A scenario frame is one emulated frame ("N emulated frames" is the user-facing contract). Dolphin consumes one movie entry per SI poll, and Melee polls the pad twice per emulated frame (a movie recorded on the developer's Dolphin has `inputCount` = 2 × `frameCount`, lag 0). So each scenario frame becomes two identical controller-1 input entries, a single named constant (`ENTRIES_PER_FRAME = 2`) in the generator. A hold of N frames writes 2N entries with those controls pressed. `wait:N` writes 2N neutral entries. Shots write no entries; they mark a frame index. Caveat: from power-on until Melee's pad init reprograms the poll rate, Dolphin polls once per frame and init commands consume a few entries, so the first input shifts by a small constant. That doesn't affect determinism, and every scenario starts with a neutral wait; slice 2 should confirm it on Dolphin.
- **Control-to-pad-state mapping (digital only, as in v1 piloting):** buttons `a b x y z start` set their button bits. `l` and `r` set the digital trigger bit and full analog trigger value. Stick and C-stick directions drive the axis to full deflection (0 or 255 around the 128 neutral). Opposing directions in one step are an error. D-pad directions set their bits. Combined controls in one step merge into one pad state. Neutral is all buttons released, both sticks centred, and triggers at 0.
- **Movie header.** Game ID `GALE01`, one standard GameCube controller on port 1, power-on start (no savestate flag), and the existing memory card used as-is (no clear-save). Input count equals the number of entries, frame count is entries ÷ 2, lag count 0. `tickCount` is set unreachably large (Dolphin ends a power-on movie once emulated ticks pass it), the MD5 is zero (skips the check; `main.dol` changes every build), the memory card bit for slot A is set, and every entry has the `is_connected` bit set. Determinism-relevant settings recorded in the header should match the configured Dolphin. The way to settle exact field values is to compare against a reference power-on movie recorded on the developer's own Dolphin.
- **CLI.**
  - `scenario <name> --movie <path>` expands the scenario, writes the movie to `<path>` and prints the path and the movie's length in frames. It needs no Dolphin, no pad bindings and no focus. It works on any platform, because it's pure logic like `--dry-run`.
  - `run --scenario <name>` generates the movie into the gitignored build output (failing before the build on any scenario error, as today), builds, replaces the tracked Dolphin as today, and launches the training build with the movie for playback. It then waits for the game window, takes each `shot` at its plan time through the existing screenshot path, and waits out the rest of the movie before returning.
  - `run --scenario <name> --live` keeps today's keyboard-driven behaviour.
  - `scenario <name>` without `--movie` or `--dry-run` keeps today's live behaviour against a running Dolphin.
- **Shots during playback.** Emulation runs at normal speed, so shot steps keep using the existing hotkey-based screenshot path, timed on the wall clock from the frame index. Shot timing is approximate, but input is exact. Taking a shot still needs focus, so the focus-permission rule is unchanged.
- **Playback mode.** The movie is played read-only, so keyboard input during playback can't append to it or override it. When it ends, the keyboard-bound controller takes over. Settle how to force read-only playback on the configured Dolphin in the first slice.
- **Savestate movies are rejected.** Generated movies are always power-on. The hand-recorded savestate movie and its `.sav` are deleted, not committed.
- **Scenario retune.** `boot-to-training` keeps one fixed-length hold per cursor axis after clamping to a corner (the 2-frame tap experiment was reverted), with hold lengths re-checked under exact frames and holds and waits shortened where frame-exact input makes the padding unnecessary. Shortening the 20-frame presses everywhere is a follow-up, not required here. Done bar: a movie run reaches Training Mode with Ness and Final Destination selected within 20 s of Dolphin launch. `boot-to-main-menu` drops its Classic-mode anchor (only there to absorb timing drift) and reaches the main menu directly.
- **Vocabulary.** Add **movie** to the glossary: a Dolphin input recording generated from a scenario, frame-exact, power-on, never committed. Update the developer workflow scenarios section accordingly. ADR 0002 (one Python entry point) holds: everything goes through `dev.py`.

## Testing Decisions

- Tests check external behaviour only: they run the real `dev.py` command line and observe exit codes, printed output and files on disk. They don't reach into the generator module's internals.
- **Seam 1: CLI without Dolphin (routine, part of the dev tests).** In a sandbox copy of the repo with fixture scenarios, as the existing scenario dry-run tests do, run `scenario <name> --movie <path>` and read the written movie back. Check the header fields (magic, game ID, power-on, controller port, counts matching the entry count), entry count equal to total scenario frames including `include`d frames, per-frame pad state for each control (buttons, triggers, stick and C-stick full deflection, D-pad), combined controls and diagonals, neutral entries for `wait`, holds landing on exactly the right frame indices, opposing-direction errors, and the existing grammar, include-cycle and missing-include errors failing without writing a file. Generating a movie must succeed with no pad bindings available.
- **Seam 2: live end-to-end (opt-in).** Extend the existing opt-in e2e test so `run --scenario` plays a small scenario as a movie and checks that its shot PNGs exist at the printed paths. It stays skipped unless the opt-in environment variable is set, and skips without a disc or Dolphin. It isn't part of `dev.py check`.
- **Reliability check (manual, by the agent with permission).** Run `run --scenario boot-to-training` several times in a row and compare the `hover-player`, `hover-final-destination` and `in-training` shots across runs. They should match every time.
- Prior art: the sandbox and CLI driver in the existing dev tests support code, the existing scenario dry-run tests (fixture scenarios, rows parsed from output), the run tests (swapping Dolphin through the dev config and restoring it), and the existing opt-in e2e test.

## Out of Scope

- Uncapped or turbo playback to shorten the boot. Shot timing relies on normal-speed emulation, so it needs frame-exact shots first.
- Frame-exact screenshots (frame dumping, or reading game state from memory).
- Savestate-based movies or scenario starting points.
- Booting straight into Training Mode through a `TRAINING_BUILD` hook, skipping menu piloting. That's a complementary idea for a separate spec, alongside ADR 0003's eventual drill mode.
- Analog stick magnitudes and modifiers, and ports other than port 1 (same limits as live piloting).
- Recording movies from live play, and editing movies by hand.
- Committing generated movies, or caching them across runs beyond the build output.
- macOS live playback. Movie generation itself is platform-independent.
- Automated image comparison of shots.

## Further Notes

- Root cause, for the record: `scenario_live` converts frames to milliseconds and delivers keystrokes on a wall clock, while Dolphin samples the host keyboard once per input poll. A press of N frames therefore lands on N±1 polls, and accelerating cursors turn that into a different menu selection. Splitting a move into short taps multiplies the error rather than cancelling it.
- Main risk: whether two input entries per frame hold through Melee's boot and loading screens, and whether training-build changes shift load or lag frames before a scenario's last input. Training-build changes that add work during scene transitions could desync a movie that used to work. Keeping movies generated from scenarios on every run, never committed, limits the damage: retiming is a text edit.
- Suggested slices: (1) confirm the frame model and header fields against a reference power-on movie on the developer's Dolphin, and implement `scenario --movie` with Seam 1 tests; (2) `run --scenario` movie playback with shots, `--live` fallback, read-only playback and the Seam 2 test; (3) retune `boot-to-training`, delete the savestate movie, and update the docs and glossary.
