# 02: Play the movie in `run --scenario`

**What to build:** `dev.py run --scenario <name>` reaches its scenario's end state by movie, not keystrokes. It generates the movie into the gitignored build output (failing before the build on any scenario error, as today), builds, replaces the tracked Dolphin as today, and boots the training build with the movie playing read-only. It waits for the game window, takes each `shot` step at its plan time through the existing screenshot path, and returns once the movie has finished. The keyboard-bound controller then takes over, so live `pad` piloting carries on from the end state. `run --scenario <name> --live` keeps today's keyboard-driven playback. `scenario <name>` against a running Dolphin keeps its live behaviour. See `../spec.md`.

This is where the spec's main risk gets settled: do two input entries per frame (01's `ENTRIES_PER_FRAME`, from Melee's twice-per-frame pad poll) hold through Melee's boot, menus and loads, including the short once-per-frame stretch before Melee's pad init? Check by playing `boot-to-main-menu` (and `boot-to-character-select`) as movies and confirming their shots. If it doesn't hold, adjust the frame-to-poll mapping from 01 so the user-facing contract ("N emulated frames") stays true. Also settle how to force read-only playback on the configured Dolphin, and report a clear error if Dolphin rejects the movie.

Live runs take focus. Ask the user before each one, per CLAUDE.md, unless they've waived it for the session.

Docs land here too: the developer workflow scenarios section explains movie playback, power-on only, the memory card assumption, `--live`, and why savestate movies run the old build's code. The glossary gains **movie** next to **scenario**.

**Blocked by:** 01 (Generate a movie from a scenario)

**Status:** resolved

- [x] `run --scenario <name>` launches the training build with the generated movie, from power-on, played read-only
- [x] Scenario errors still fail before anything is built or launched
- [x] `shot` steps produce labelled PNGs at the printed paths during playback
- [x] The command returns after the movie's last frame (plus a small margin), and the keyboard-bound controller works afterwards
- [x] `--live` gives today's keyboard playback unchanged; `scenario <name>` without `--movie`/`--dry-run` is unchanged
- [x] A movie Dolphin rejects (or a missing movie option) gives a clear error, not a silent keyboard fallback
- [x] `boot-to-main-menu` and `boot-to-character-select` reach their expected shots as movies (frame model confirmed or corrected)
- [x] The opt-in e2e test plays a small scenario as a movie and checks its shot PNGs exist; still skipped without the opt-in variable, a disc or Dolphin; not part of `dev.py check`
- [x] Developer workflow doc and glossary updated; `dev.py check` passes

## Findings (settled live on Dolphin 2609)

- **Frame model holds:** `ENTRIES_PER_FRAME = 2` is right through boot, menus and loads. `boot-to-character-select` and `boot-to-training` reach every shot as movies with no retiming (Ness, Final Destination, Training Mode on the first run). In the main menu, four 2-frame `stick-down` presses 6 frames apart all register, every run. The once-per-frame boot stretch before Melee's pad init is not visible in practice (it sits inside the opening neutral wait).
- **Boot timeline (movie frames from power-on):** black at frame 30, Nintendo/HAL logo from at least frame 120 to 240, intro movie playing by frame 300 (shots every 30 frames). `start:2` at frame 330 shows the title screen by frame 392; a second `start:2` at frame 392 reaches the main menu (with the memory card "SAVING" icon) by frame 454. So the main menu is reachable around frame 450 versus 1640 for the current Classic-mode anchor.
- **Shot timing:** shots land about 1 to 7 frames after their nominal frame, with no drift over 30 s (probes every 300 frames). The window lookup polls every 50 ms so the start of the clock is tight; `MOVIE_LEAD_S` stays 0.
- **Read-only:** command-line `-m` playback is always read-only (Dolphin's default, not an ini setting); `run` also passes `-C Dolphin.Movie.PauseMovie=False`. After the movie ends, `pad` input moves Ness (keyboard takeover confirmed).
- **Rejection:** a bad magic or a wrong game ID makes Dolphin open a modal "Warning" dialog (the game doesn't boot for a bad magic). `run` detects a non-"Dolphin..." window of its Dolphin and fails naming it. A Dolphin without `--movie`, and RetroAchievements hardcore mode (silent refusal), are rejected before the build.
- **Movie file:** written to `build/training/movies/<name>.dtm` after the previous Dolphin is stopped (a Dolphin blocked on a rejection dialog still holds its movie file open).
