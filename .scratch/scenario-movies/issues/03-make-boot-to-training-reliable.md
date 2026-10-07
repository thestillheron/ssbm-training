# 03: Make `boot-to-training` run the same every time

**What to build:** `boot-to-training` played as a movie lands on Ness and Final Destination every run, and gets faster where exact input makes the safety padding unnecessary. The cursor moves stay as single fixed-length holds per axis after clamping to a screen corner (the 2-frame tap trains are already reverted). Retune the hold lengths for exact frames if they now land elsewhere, and shorten waits and the 20-frame presses within this scenario where the menus allow. Shortening presses across all scenarios is a follow-up, not required here. Remove the hand-recorded savestate movie and its `.sav` from the scenarios folder: savestate movies restore the old build's code and aren't a supported route. See `../spec.md`.

Live runs take focus. Ask the user before running, per CLAUDE.md, unless they've waived it for the session.

**Done bar (from the developer):** tuning is finished when a movie run gets from Dolphin launch into Training Mode, with Ness and Final Destination correctly selected, in **20 seconds or less**.

This includes `boot-to-main-menu`. Its current anchor trick (press start repeatedly into Classic mode's character select, then hold B back out to the main menu) only existed to absorb wall-clock timing drift against the attract loop. With frame-exact input it isn't needed: skip the intro and title at fixed frames and go straight to the main menu.

**Blocked by:** 02 (Play the movie in `run --scenario`), 04 (Fixed memory card for movie runs)

**Status:** resolved

- [x] A movie run reaches Training Mode (Ness, Final Destination, both fighters on stage) within 20 s of Dolphin launch
- [x] `boot-to-main-menu` no longer uses the Classic-mode anchor; it reaches the main menu directly at fixed frames
- [x] At least three back-to-back `run --scenario boot-to-training` runs give matching `hover-player`, `hover-final-destination` and `in-training` shots (Ness picked, Final Destination picked, both fighters on stage)
- [x] Waits and holds trimmed where safe; the scenario's comments explain the timing in frame-exact terms (no more "a key press can be dropped" rationale)
- [x] The drill scenarios that `include` it (tech-chase ones) still reach their states
- [x] The savestate movie and its `.sav` are deleted from the scenarios folder
- [x] `dev.py check` passes

## Findings (measured live on Dolphin 2609, fixed memory card)

- **Fast disc speed was needed for the 20 s bar.** With the header's emulated disc speed (as recorded on the developer's Dolphin), the main menu reads input only from frame 638 and the 1-P Training -> character select load alone is about 200 frames, so Training on Final Destination lands around 26 s after launch at best. The movie header now sets `bFastDiscSpeed` (playback applies the header's settings, so the developer's Dolphin.ini is untouched). That changes every load; input stays frame-exact and runs stay identical. Test: `test_header_turns_on_fast_disc_speed`.
- **Earliest registering presses (movie frames from power-on, 2-frame presses, bisected one frame apart):** `start` skipping logo and intro to the title from 62 (61 is ignored); with that `start` at 66, the title's `start` from 89 (88 ignored); main menu `a` from 122 (121 ignored); with `a` at 125, the 1-P submenu `stick-up` from 131 (130 ignored), and up wraps from Regular Match to Training; Training's `a` registers already at 137, one frame after the up is released; on character select, `start` after picking Ness registers one frame after the `a` is released.
- **Cursor clamps:** the character select hand reaches the top-left corner by frame 216 from a clamp starting at 150 (66 frames; the screen starts reading the stick around 155-160, so a clamp may start before it); the stage select crosshair reaches the bottom-left corner 35 frames after a clamp starting at 270. The fixed holds (`stick-right:14`, `stick-down:13` onto Ness; `stick-right:22`, `stick-up:8` onto Final Destination) land the same with 4-frame neutral gaps as with the old 180-frame waits.
- **Committed timeline (each press a few frames after its earliest frame):** `start` 66 and 92, main menu at 125; `a` 125, `stick-up` 134, `a` 140; hand clamp 150-222, Ness at 261; `start` 266; crosshair clamp 270-310, Final Destination at 352; stage drawn with Ness by about 456, CPU on screen by about 514; the movie ends at 534.
- **Wall time:** Dolphin launch -> game window 2.6 to 2.75 s, so Training (both fighters on stage, frame 534) is reached 11.5 to 11.7 s after launch (3 back-to-back runs: 11.51, 11.66, 11.49 s; overlay checked at the computed time: field 551 = frame 533). The three runs' `hover-player`, `hover-final-destination` and `in-training` shots match (Ness, Final Destination, Ness and the CPU, Pichu in all three, on stage).
- `boot-to-character-select` (VS. Mode character select by frame 205), `e2e-title` (title by frame 128), `tech-chase-loop` and `tech-chase-displace` were each played once and reached their states.

## Review follow-up: load margins (measured live on Dolphin 2609)

- **Retimed for margin.** The presses above sat only 3 or 4 frames past their earliest registering frame, so a training-build change that lengthened a load by a few frames would desync the movie. Every press or clamp that waits on a load now sits about 10 frames or more past what was measured: `start` 72 and 105, main menu ready at 145; 1-P `a` 145, `stick-up` 161 (earliest 151), `a` 167; hand clamp 177-259 (82 frames; the corner is reached 66 frames into a clamp); Ness at 298; `start` 303; crosshair clamp 307-357 (50 frames; corner after 35); Final Destination at 399; the movie ends at 581 (CPU on screen about 162 frames after the pick). `boot-to-character-select`'s submenu `a` now comes 16 frames after the one before (earliest 6), and `e2e-title`'s `start` is at 72.
- **Shot timing re-measured.** A shot every 62 frames against a 2-frame `stick-down` every 8 frames in the main menu: with shots sent at window + N fields, each shot caught movie frame N to N+4. Adding the 18-frame boot offset (ticket 04) to shot times made them land about 20 frames late (hover shots showed the next screen), so the clock's start (the game window appearing) already absorbs it; `follow_movie` uses the offset only for the end.
- **Wall time:** 4 back-to-back `run --scenario boot-to-training` runs: game window 2.57, 2.60, 3.15, 2.77 s after launch, so Training (frame 581) is on screen 12.26, 12.29, 12.84, 12.47 s after launch. `in-training` shots byte-identical in all 4; `hover-player` (Ness picked, CPU Pichu) and `hover-final-destination` (Final Destination) the same picture in all 4, differing only by a few frames of animation.
- `boot-to-character-select` (window 2.66 s, character select at frame 232, 6.5 s), `e2e-title` (title shot), `tech-chase-loop` (tech in place and resets), `tech-chase-displace` (hits land: damage 11 at the end, player back at start) and `tech-in-place` (first live check: drop in tumble and reset reps) each played once and reached their states.
