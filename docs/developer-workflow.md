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

Frames convert to time at 60 fps, so timing is approximate, not frame-exact. The controls are `a b x y z start l r`, `stick-up/down/left/right`, `cstick-up/down/left/right` and `dpad-up/down/left/right`.

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
- The game's attract loop (intro movie, title, demos) runs on its own: the "- T" marker is drawn on the title screen, and the title screen only appears about 85 s after boot and then again after each demo cycle. Take several shots, or press `start` with `pad`, rather than expecting one shot to catch it.

### Scenarios {#developer_workflow_scenarios}

A [scenario](glossary.md#glossary_scenario) is a committed, named piloting script: `tools/scenarios/<name>.txt`, played with

```
python dev.py scenario <name> --dry-run
```

`--dry-run` expands the scenario and prints its full timed plan (key events and shot steps) without touching Dolphin. Without it, `scenario` plays the plan live against the running training build through the same focus-guarded input path as `pad` (so the same focus permission rule applies), taking each `shot` step through `shot` and printing every shot path. Any abort releases held keys and restores focus. Files are plain text, one step per line, using exactly the `pad` step grammar plus:

- `shot <label>`: take a screenshot at this point.
- `include <name>`: expand another scenario here. Includes are resolved before any input is sent; a cycle or a missing scenario is an error naming the include chain.
- `# ...`: comment. Blank lines are ignored.

The first scenario, `boot-to-character-select`, is a reusable prefix: it waits out the intro movie (the title screen appears about 85 s after boot), presses `start` twice to reach the main menu, then goes VS. Mode to character select, with shots `title`, `main-menu` and `character-select`.

```
python dev.py run --scenario boot-to-character-select
```

`run --scenario <name>` goes from a code change to a verified game state in one command: it checks the scenario first (an unknown scenario or a bad file fails before anything is built), then builds, launches as plain `run` does, waits up to 30 s for the game window, and plays the scenario. If the window doesn't appear in time it fails with a clear error and sends nothing. The Dolphin keeps running afterwards.

Codifying piloting: once a `pad` sequence you worked out by hand is worth repeating, paste its steps into a new file in `tools/scenarios/`, one per line, add comments saying what each part does, `include` a prefix such as `boot-to-character-select` instead of repeating it, add `shot` steps at the checkpoints, and check it with `--dry-run`.

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

The end-to-end test (`tools/dev_tests/test_e2e.py`) runs `run --scenario` against a real Dolphin: it boots the training build, takes a shot of the title screen, checks the PNG exists, then runs again to confirm the previous Dolphin was replaced. It takes window focus and sends keystrokes, takes a few minutes, and is not part of `dev.py check`. It is skipped unless you opt in with `SSBM_E2E=1` (and also skips without a disc image or Dolphin):

```
SSBM_E2E=1 python -m unittest tools.dev_tests.test_e2e      # macOS / bash
$env:SSBM_E2E = "1"; py -m unittest tools.dev_tests.test_e2e  # Windows PowerShell
```

Don't touch the keyboard or other windows while it runs. It terminates the Dolphin it launched when it finishes.
