# 03: `shot`

**What to build:** `dev.py shot [label]` takes a screenshot of the running training build's game frame using Dolphin's own screenshot hotkey, moves the file to a predictable place and prints its path, so the agent can read the image straight away. See the spec (`../spec.md`).

**Blocked by:** 02 (Live `pad`).

**Status:** resolved

- [x] Sends Dolphin's screenshot hotkey through the same focus-guarded input path as `pad`. The key comes from Dolphin's hotkey config if bound there, otherwise the default F9.
- [x] Waits (with a timeout and a clear error) for a new file in Dolphin's screenshots folder, then moves it into the training build's shots folder (under the gitignored build output) as `<label>.png`, or a timestamp name when no label is given.
- [x] Prints the absolute path of the moved PNG.
- [x] `run` clears the shots folder at each launch.
- [x] On macOS, `shot` exits with a clear "not supported yet" message.
- [x] Verified by hand: a shot of the title screen shows the "- T" marker.
- [x] The developer workflow doc's piloting section describes `shot` and where shots land.

## Answer

Implemented `dev.py shot [label] [--timeout S] [--dry-run]`.

For ticket 05 (scenario `shot` steps), in `tools/shot.py` (no Win32; takes the same backend as `pad_live.run_plan`):

- `shot.take_shot(label, key, hwnd, backend, screenshots, shots_dir, timeout=10.0, sleep=, clock=) -> Path`: presses the hotkey through `pad_live.run_plan`, waits for a new PNG under `screenshots`, moves it to `shots_dir/<label>.png` (timestamp name if `label` is None) and returns the absolute path. Raises `PadError` on timeout or bad label.
- `shot.screenshot_key(user_dir) -> Key` (Hotkeys.ini `General/Take Screenshot`, default F9; modifier chords are an error), `shot.screenshots_dir(user_dir)`, `shot.shot_target(label, shots_dir)`, `shot.clear_shots(shots_dir)`.
- In `dev.py`: `SHOTS_DIR` (`build/training/shots`) and `cmd_shot` show the wiring: `win_pilot.find_dolphin_window(state pid, config title)` then `take_shot(..., win_pilot.Win32Backend(), shot.screenshots_dir(dolphin_user_dir()), SHOTS_DIR)`. A scenario player can call `take_shot` per `shot` step (each call focuses and restores).
- `run` calls `shot.clear_shots(SHOTS_DIR)` at launch.

Tests: `tools/dev_tests/test_shot.py` (CLI `--dry-run` with fixture user dir, `take_shot` with a fake Dolphin backend, timeout, clearing) and `test_run.py::test_run_clears_the_shots_folder` (needs a disc).

Hand verification (user waived focus): after `run`, `shot` produced PNGs and the second title screen shows the "- T" marker beside "SUPER". The first title appears ~85 s after boot, after the intro movie and demos, so a shot right after boot shows the movie.
