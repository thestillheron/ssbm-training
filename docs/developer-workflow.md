# Developer workflow {#developer_workflow}

This fork adds training features to the Melee decompilation. This page takes you from a fresh clone to Melee's title screen showing a "- T" marker, which tells you Dolphin booted your build and not the original game. You don't need to know decomp first; @ref getting_started covers the upstream project.

Every command is run through `dev.py` at the repo root. Use `python dev.py ...` on macOS and `py dev.py ...` on Windows (the examples below say `python`). The commands are otherwise identical on both.

## Two builds

- The [matching build](glossary.md#glossary_matching_build) is the unmodified game. Its `main.dol` must be byte-identical to the original. It is built into `build/GALE01/`, upstream's default location.
- The [training build](glossary.md#glossary_training_build) is the game plus this fork's training features. It is built into `build/training/`.

Why we keep both: [ADR-0001](adr/0001-matching-build-alongside-modded-build.md). Why the tooling is one Python script on each OS's native toolchain: [ADR-0002](adr/0002-native-toolchain-behind-one-python-entry-point.md).

## Prerequisites

- Python 3 (needed to run `dev.py`, which uses only the standard library). It creates a project virtual environment `.venv` and installs ninja and the repo's Python requirements (`reqs/build.txt`) into it, so nothing is installed globally.
- Windows: nothing else is needed to build. No WSL, msys2 or admin rights.
- macOS (Apple Silicon is supported): Wine, to run the Windows-only MWCC compiler. `setup` checks for it and fails if it is missing, printing:
  ```
  brew install --cask --no-quarantine wine-stable
  softwareupdate --install-rosetta
  ```
  (the second line is for Apple Silicon).
- Dolphin (vanilla), only for `run`. `setup` looks for it and prints how to install it if it can't find it. See [Dolphin](#developer_workflow_dolphin).

## Supply a disc image

The game's data can't be committed to a public repo, so you provide your own copy. It must be **Melee NTSC-U 1.02 (`GALE01`)**. Other regions and versions are not supported. RVZ and ISO both work (the tool also accepts `.gcm`, `.wia`, `.ciso`, `.gcz` and `.nfs`).

Do one of the following:

- Put the image in `orig/GALE01/` (gitignored). `setup` uses it if it is the only disc image there.
- Pass the path: `python dev.py setup path/to/Melee.rvz`.
- Set `"disc"` in `dev.config.json` (see [Per-machine config](#developer_workflow_config)).

## Commands

### `setup`

```
python dev.py setup [disc image] [--force]
```

- Detects Dolphin (and Wine on macOS).
- Downloads decomp-toolkit if needed, extracts the whole disc into `orig/GALE01/sys` and `orig/GALE01/files`, and verifies `main.dol` against the SHA-1 in `config/GALE01/build.sha1`. A wrong disc fails here with the version message.
- Creates `.venv` and installs ninja and the requirements.

It is safe to re-run. If the disc is already extracted and verified it skips extraction; `--force` re-extracts.

### `build`

```
python dev.py build             # training build -> build/training/main.dol
python dev.py build --matching  # matching build -> build/GALE01/main.dol
```

Both builds share one `build.ninja` at the repo root. `dev.py` remembers which build is configured in `build/dev_state.json` and re-runs `configure.py` only when you switch builds or `configure.py` changes, so switching doesn't recompile untouched code. The matching build's own ninja `check` step verifies the SHA-1, so a successful `build --matching` means it is byte-identical. The training build is configured with `configure.py --training`.

### `run`

```
python dev.py run
```

Builds the training build, assembles a game folder in `build/training/game/`, and launches Dolphin on its `main.dol`; it doesn't wait for Dolphin to exit. The folder mirrors your extracted disc using hard links (falling back to copies, with a warning, if linking fails, e.g. across drives) and replaces only `main.dol` with a fresh copy of the training build's. Files under `orig/` are never written to. You should see "- T" next to the "Melee" subtitle on the title screen.

`run --scenario <name>` also boots with a scenario played as a movie from power-on; see [Scenarios](#developer_workflow_scenarios).

### `check`

```
python dev.py check
```

Builds the matching build, then the training build (even if the first fails), and prints a `check summary` with `matching: PASS|FAIL` and `training: PASS|FAIL`. It exits non-zero if either fails. Matching must pass its SHA-1 verification; training must compile and link.

**Run `check` before merging to `master`.** The fork has no CI (the game's `main.dol` can't be stored publicly), so this local gate is the only check.

## Piloting {#developer_workflow_piloting}

[Piloting](glossary.md#glossary_piloting) is driving the running training build with controller input. It is Windows only for now; on macOS `pad` exits with "not supported yet".

```
python dev.py pad "<sequence>" --dry-run
```

`--dry-run` prints the ordered, timed key-down/key-up plan the sequence resolves to, and touches no Dolphin. Without it, `pad` brings the Dolphin window to the front, sends the keys with `SendInput` scancodes, and restores the previously focused window.

**Focus permission:** `run` and live `pad` take window focus (and `pad` sends keystrokes). Ask the user before running either, unless they have waived that for the current session. Use `--dry-run` freely.

Safety behaviour of live `pad`:

- The window is found by title among the windows of the Dolphin that `run` launched, falling back to title matching. Zero or several matches is an error and nothing is sent. The title is the regex `dolphin_window_title` (default `^Dolphin .*\|`, which matches the game window and not the bare main frame).
- Focus is taken only when a key event is due, never during idle waits or between shots. Before every key event Dolphin must be the foreground window: if it is not and no key is held it is brought back, otherwise all held keys are released and `pad` exits non-zero with "focus lost".
- Windows may refuse a background process the foreground. When the first attempt fails, the focus step taps Alt (which lands on the current window and does nothing there) to lift the foreground lock, then tries once more.
- Held keys are released and focus restored on success, on every failure and on Ctrl+C.
- `run` records the PID of the Dolphin it launches in `build/dev_state.json` and, on the next `run`, terminates that process (only if it is still a Dolphin process) before launching. Other Dolphin processes are never touched.

A sequence is steps separated by whitespace. A step is either:

- controls joined by `+`, with an optional hold in frames after a colon: `a`, `stick-down+b:6`. The default hold is 3 frames.
- `wait:N`, idle for N frames.

Frames convert to time at 60 fps, so timing is approximate, not frame-exact (a scenario played as a movie by `run --scenario` is frame-exact). The controls are `a b x y z start l r`, `stick-up/down/left/right`, `cstick-up/down/left/right` and `dpad-up/down/left/right`.

Controls are translated to keyboard keys through the `[GCPad1]` section of Dolphin's `GCPadNew.ini`, read fresh on every command (see `dolphin_user` under [Per-machine config](#developer_workflow_config)). GameCube port 1 must be bound to the keyboard, and every control a sequence uses must be bound; otherwise `pad` fails with an error naming the problem.

### Shots {#developer_workflow_shots}

```
python dev.py shot [label] [--timeout SECONDS] [--dry-run]
```

`shot` screenshots the running training build so you can read the image. It sends Dolphin's own screenshot hotkey through the same focus-guarded input path as `pad` (so it takes window focus: same permission rule), waits for the new PNG in Dolphin's `ScreenShots` folder (default timeout 10 s, with a clear error), moves it to `build/training/shots/<label>.png` (a timestamp name if no label; a repeated label replaces the earlier file) and prints the absolute path. Labels use letters, digits, `.`, `_` and `-`.

- The hotkey is `General/Take Screenshot` in Dolphin's `Hotkeys.ini` (in the `dolphin_user` folder), default F9. It must be a single keyboard key; modifier chords are an error.
- `run` clears `build/training/shots/` at every launch. The folder is under gitignored `build/`.
- `--dry-run` prints the hotkey and paths and touches no Dolphin.
- On macOS `shot` exits with "not supported yet".
- The game's attract loop (intro movie, title, demos) runs on its own: the "- T" marker is drawn on the title screen, and without input the title screen only appears about 85 s after boot and then again after each demo cycle. Take several shots, or press `start` with `pad`, rather than expecting one shot to catch it. (In a movie, a `start` from frame 62 skips the logo and intro to the title, as `e2e-title` does.)
- Only one Dolphin should be running while you use `shot`. Dolphin instances share one user folder, so a second one (another session's, or your own main Dolphin) writes into the same `ScreenShots` folder and `shot` can return that instance's picture under your label. A standalone `dev.py shot` while another window is in front can also miss (focus is handed back right after the key press, before Dolphin polls it), so for shots of a moving scene prefer `shot` steps inside a scenario.

### Scenarios {#developer_workflow_scenarios}

A [scenario](glossary.md#glossary_scenario) is a committed, named piloting script: `tools/scenarios/<name>.txt`. Files are plain text, one step per line, using exactly the `pad` step grammar plus:

- `shot <label>`: take a screenshot at this point.
- `include <name>`: expand another scenario here. Includes are resolved before any input is sent; a cycle or a missing scenario is an error naming the include chain.
- `# ...`: comment. Blank lines are ignored.

The usual way to play one is from power-on, as a [movie](glossary.md#glossary_movie):

```
python dev.py run --scenario boot-to-training
```

`run --scenario <name>` goes from a code change to a verified game state in one command:

1. It expands the scenario first: an unknown scenario, a bad line, a missing include or a cycle fails before anything is built or launched. So does a Dolphin that can't play movies (no `-m`/`--movie` option, or RetroAchievements hardcore mode on, in which Dolphin silently refuses movies).
2. It snapshots your slot A memory card if there is no snapshot yet (see **Memory card** below). It builds, stops the Dolphin of the last `run`, writes the movie to `build/training/movies/<name>.dtm` (gitignored, regenerated on every run, never committed) and makes a fresh run copy of the card snapshot.
3. It launches `Dolphin -e <main.dol> -m <movie>` with slot A pointed at the run copy, so the training build boots with the movie playing from power-on. Command-line playback is always read-only (Dolphin's default; no ini setting changes it), so the keyboard can't override or append to the movie input while it plays. `run` also passes `-C Dolphin.Movie.PauseMovie=False` so emulation carries on when the movie ends.
4. It waits up to 30 s for the game window, takes each `shot` step at its frame on the wall clock (frame / 59.94 s after the window appears) through the same path as `shot`, printing every path, and returns about 1 s after the movie's last frame, printing "Movie finished". The end allows for the boot offset: until Melee sets up the pad, Dolphin polls it once per frame instead of twice, so frame N of the movie is drawn about 18 frames after power-on + N. (Shots need no such allowance: the window appears, and a shot takes effect, about that much after power-on too.) From then on port 1 is the keyboard-bound controller again, so live `pad` piloting carries on from the reached state.

If Dolphin rejects the movie it opens a modal dialog (for example "Warning" for a bad file or another game's movie); `run` reports that dialog as an error instead of waiting or falling back to the keyboard. If the window doesn't appear in time it fails with a clear error. The Dolphin keeps running afterwards. `run` still takes focus (for the launch and the shots), so the focus permission rule applies.

What movie playback means for scenario authors:

- **Frames are exact.** A hold of N frames is exactly N emulated frames of input, and `wait:N` exactly N neutral frames. Melee polls the pad twice per emulated frame, so each scenario frame is two identical movie entries. This holds through boot, menus and loads (checked live: four 2-frame presses 6 frames apart in the main menu all register, every run). Shot timing is only approximate: a shot lands a few frames (measured 0 to 4) after its nominal frame, so a shot right before a press can show that press's effect.
- **Power-on only.** A movie always starts at power-on. Never start a movie from a savestate: a savestate restores all of RAM, including the game code, so a savestate movie silently runs the build it was recorded with, not the one you just built.
- **Memory card.** Every movie run starts from the same card contents: a [memory card snapshot](glossary.md#glossary_memory_card_snapshot) of your slot A card, `build/memcard/snapshot.USA.raw` (gitignored, never committed). The first `run --scenario` copies it from the raw card file Dolphin has in slot A and prints where it is. Before each launch `run` copies the snapshot to a throwaway [run copy](glossary.md#glossary_run_copy), `build/memcard/run.USA.raw`, and points slot A at that copy for this launch only (`-C Dolphin.Core.SlotA=1 -C Dolphin.Core.MemcardAPath=...`; your Dolphin.ini is not edited). Melee saves to the card during play (the "SAVING" icon at the main menu), but only the run copy changes: your real card is never written by a movie run, and the same build and scenario always start from the same card. Slot A must be a raw memory card file (Dolphin: Config > GameCube > Slot A: Memory Card); a GCI folder or no card is an error naming the problem. Scenarios assume the snapshot holds a save with every character and stage unlocked: the character select and stage select layouts, and so the cursor moves, depend on it. The training build does not unlock anything itself. With a fresh save Final Destination shows as "?" and the moves land elsewhere. After you unlock things on your real card (or switch cards), refresh the snapshot with `python dev.py snapshot-card`. `--live` runs and `scenario <name>` use Dolphin's own card as before.
- **Fast disc loads.** The movie header turns on Dolphin's fast disc speed (playback applies the header's settings, whatever your Dolphin.ini says). Loads take a fraction of the emulated disc time, so the boot timeline is: a `start` from frame 62 skips the logo and intro to the title, a `start` on the title from 23 frames after that goes to the main menu, and the main menu reads input from 30 frames after the title's `start` (measured earliest frames, with presses at 66 and 92: 89 and 122). Without it every load (main menu, character select, stage) took seconds, and the main menu only read input from frame 638. The frame numbers in the committed scenarios assume fast disc.
- **Load margins.** A training-build change that adds work to a scene transition makes that load, and every screen after it, a few frames later, and a press that used to land just after a screen became ready would then land before it and be lost. So in the committed prefixes every press or cursor clamp that waits on a load sits about 10 frames or more after the earliest frame it was measured to work at. Keep that margin when you retime them. If a prefix stops reaching its shots after a change, look for a load that got longer.
- **Header settings are the developer's.** Apart from fast disc, the movie header's settings (video backend, CPU core and the other emulation settings, Dolphin revision, start time, country code) are copied from a reference movie that the developer's Dolphin 2609 recorded, and playback applies them. That is a known single-machine assumption: the scenario frame numbers were measured under these settings, so another Dolphin version may need a new reference movie (see `tools/movie.py`) and retuned scenarios.
- **Pad bindings don't matter.** Movie input never goes through the keyboard, so a movie needs no `GCPadNew.ini` bindings, and stray keystrokes or focus changes during playback can't change the input. Dolphin hotkeys still work, so don't press its savestate keys (F1-F8) during a run: loading a state ends or desyncs playback.
- Character select and stage select cursors accelerate the longer the stick is held, so move them by clamping to a screen corner, waiting, then holding one direction for a fixed number of frames. With exact frames the same move lands on the same place every run.

`run --scenario <name> --live` is the old keyboard route: it launches without a movie, waits for the game window and sends the scenario as keystrokes on the wall clock, like `scenario <name>` below. It needs the pad bindings and takes focus for every key, and a hold of N frames lands on N-1 to N+1 frames, so cursor moves can drift. Use it to compare, or as a fallback for short scenarios if movie playback misbehaves. It is for short scenarios against the game once it is up, not for the committed boot prefixes: those are timed for movies (fast disc loads, 2-frame presses a few frames apart), which keystrokes on the wall clock can't follow, so `boot-to-*` and the drills that include them won't get through the menus live.

```
python dev.py scenario <name> --dry-run
python dev.py scenario <name> --movie <path>
python dev.py scenario <name>
```

`--dry-run` expands the scenario and prints its full timed plan (key events and shot steps) without touching Dolphin. `--movie <path>` writes the scenario as a movie to `<path>` and prints its path and length in frames; like `--dry-run` it needs no Dolphin, no pad bindings and no focus, and works on any OS. With neither, `scenario` plays the plan live as keystrokes against an already-running training build (for a short scenario mid-session, where a power-on movie can't apply), through the same focus-guarded input path as `pad` (so the same focus permission rule applies), taking each `shot` step through `shot` and printing every shot path. Any abort releases held keys and restores focus.

The committed prefixes:

- `boot-to-main-menu`: power-on to the main menu, ready for input with the cursor on 1-P Mode, at frame 145 (about 2.5 s of movie): two `start` presses at fixed frames skip the intro and the title.
- `boot-to-character-select`: `boot-to-main-menu`, then VS. Mode to character select, with shots `main-menu` and `character-select`.
- `boot-to-training`: the prefix for drill scenarios. It goes boot, main menu, 1-P Mode, Training, picks Ness for the player, and ends in Training Mode on Final Destination with both fighters on stage at frame 581 (about 9.7 s of movie). Measured on the developer's machine, Training is on screen 12.3 to 12.8 s after Dolphin launch (the game window appears 2.6 to 3.2 s after launch). The CPU opponent keeps whatever character Training's character select gives it.

The committed drill scenarios:

- `tech-chase-first-rep`: the acceptance scenario for the tech-chase drill's first rep. It includes `boot-to-training`, then takes shots of the opponent in tumble, during its tech in place, and after the reset with the second rep starting. Read the shots in order to confirm each phase. Run it with `python dev.py run --scenario tech-chase-first-rep`.
- `tech-chase-displace`: a development aid, not an acceptance check. With the rep loop running, the player runs at the opponent and hits or grabs it, to check every reset still starts a clean rep.

Codifying piloting: once a `pad` sequence you worked out by hand is worth repeating, paste its steps into a new file in `tools/scenarios/`, one per line, add comments saying what each part does, `include` a prefix such as `boot-to-training` instead of repeating it, add `shot` steps at the checkpoints, check it with `--dry-run`, then play it with `run --scenario`.

## Where training code goes

- New training code lives in `src/training/`. Add each new file to the `training` library in `configure.py` (the block under `if args.training:`); it is only compiled for the training build.
- Wrap the contents of every `src/training/` file in `#ifdef TRAINING_BUILD ... #endif`, as `src/training/title_marker.c` does.
- To hook training code into upstream game code, add a single guarded line in the upstream file. `configure.py --training` defines `TRAINING_BUILD` for every compiled unit; the matching build never defines it, so the hook compiles out entirely. The existing example is in `gm_Scene_Title_OnEnter` in `src/melee/gm/gmtitle.c`, which draws the "- T" marker:

  ```c
  #ifdef TRAINING_BUILD
      { extern void training_title_marker(void); training_title_marker(); }
  #endif
  ```

Keeping hooks to one guarded line makes them easy to find and re-apply after merging upstream `doldecomp/melee`. If you forget a guard, the matching build stops being byte-identical and `check` fails.

## Per-machine config {#developer_workflow_config}

`dev.config.json` at the repo root is gitignored and holds this machine's overrides. All keys are optional:

```json
{
  "disc": "D:/games/Melee.rvz",
  "wine": "/opt/homebrew/bin/wine",
  "dolphin": "C:/Tools/Dolphin/Dolphin.exe",
  "dolphin_user": "C:/Tools/Dolphin/User",
  "dolphin_window_title": "^Dolphin .*\\|"
}
```

- `disc`: disc image for `setup` (a command-line path takes precedence).
- `wine`: Wine executable. Used on macOS; on other systems it is only looked up if the key is present.
- `dolphin`: Dolphin executable used by `run`. A configured path that doesn't exist is an error; it is not silently replaced by auto-detection.
- `dolphin_user`: Dolphin's user folder, where `pad` reads the controller bindings. Without it, the `User` folder beside the configured Dolphin is used if a `portable.txt` sits next to it, otherwise the standard per-user location (`%APPDATA%\Dolphin Emulator` on Windows).
- `dolphin_window_title`: regex matched against window titles to find Dolphin for `pad` (default `^Dolphin .*\|`, which matches the game window "Dolphin ... | backend | game" and not the bare main frame). Change it if your Dolphin's window title differs.
- `detected`: written by `setup` to record what it found. Don't edit it.

## Dolphin {#developer_workflow_dolphin}

`run` uses your normal Dolphin install and settings, launching it as `Dolphin -e <main.dol>`. Vanilla Dolphin is the supported emulator. Slippi Dolphin is not a goal; if you want to try a different executable, point `dolphin` in `dev.config.json` at it. Without an override, `setup` and `run` look in the standard locations: `/Applications/Dolphin.app` or `~/Applications/Dolphin.app` on macOS; Program Files, `%LOCALAPPDATA%`, `%APPDATA%` and `~/Dolphin-x64` on Windows; then `Dolphin` or `dolphin-emu` on `PATH`.

## Troubleshooting

- **"... is not the right game: main.dol does not match the recorded SHA-1".** The disc isn't NTSC-U 1.02 (`GALE01`). Use that version.
- **"more than one disc image in orig/GALE01".** Leave one there, or pass the one you want to `setup`.
- **"Automatic extraction failed".** The image may be corrupt or unsupported. The printed fallback is to extract the whole disc from Dolphin (right-click the game, Properties, Filesystem, right-click Disc, Extract Entire Disc) into `orig/GALE01`, then re-run `setup`.
- **Wine stops launching after a macOS upgrade.** Clear its quarantine flag, adjusting the path to your install: `xattr -dr com.apple.quarantine /Applications/Wine*.app` (this is the command `setup` prints).
- **"Wine is required on macOS".** Install it as shown under Prerequisites, or set `wine` in `dev.config.json`.
- **"could not find Dolphin".** Install Dolphin from <https://dolphin-emu.org/download/> (on macOS also `brew install --cask dolphin`), or set `dolphin` in `dev.config.json`.
- **"run `python dev.py setup <disc image>` first".** `build` needs the extracted disc and `.venv` that `setup` creates.

## Tests for the tooling

`tools/dev_tests/` contains `unittest` tests that drive the real `dev.py` commands. Tests that need the game's data skip when no disc image is present.

The end-to-end test (`tools/dev_tests/test_e2e.py`) runs `run --scenario` against a real Dolphin: it boots the training build with the `e2e-title` movie (which presses `start` to skip the intro), checks that the movie finished and the title shot's PNG exists, then runs `e2e-smoke` with `--live` to cover keyboard playback and to confirm the previous Dolphin was replaced. It takes window focus and sends keystrokes, usually takes under a minute (it fails if the movie `run`, build included, takes two minutes or more), and is not part of `dev.py check`. It is skipped unless you opt in with `SSBM_E2E=1` (and also skips without a disc image or Dolphin):

```
SSBM_E2E=1 python -m unittest tools.dev_tests.test_e2e      # macOS / bash
$env:SSBM_E2E = "1"; py -m unittest tools.dev_tests.test_e2e  # Windows PowerShell
```

Don't touch the keyboard or other windows while it runs. It terminates the Dolphin it launched when it finishes.
