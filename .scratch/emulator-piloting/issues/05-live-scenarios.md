# 05: Live scenarios and `run --scenario`, plus the end-to-end test

**What to build:** `dev.py scenario <name>` plays a scenario live against the running training build, taking its shots along the way. `dev.py run --scenario <name>` goes from a code change to a verified game state in one command: build, launch, wait for the window, play. An opt-in end-to-end test exercises the live path against real Dolphin. See the spec (`../spec.md`).

**Blocked by:** 02 (Live `pad`), 03 (`shot`), 04 (Scenario files).

**Status:** resolved

- [x] `dev.py scenario <name>` plays the expanded plan through the focus-guarded input path, taking each `shot` step through the `shot` behaviour and printing every shot path.
- [x] `dev.py run --scenario <name>` launches as `run` does, waits for the game window (timeout about 30s, clear error, nothing sent if it times out), then plays the scenario.
- [x] Any abort mid-scenario releases held keys and restores focus, as with `pad`.
- [x] The first scenario's waits are tuned so it reliably reaches character select, confirmed with a shot (ask the user before taking focus).
- [x] Opt-in end-to-end test (Seam 2): it's skipped unless an explicit environment variable is set, and also skips without a disc or Dolphin. It runs `run --scenario` with a small scenario that reaches the title screen and takes a shot, checks the PNG exists at the printed path, and runs again to confirm the previous instance was replaced. It isn't part of `dev.py check`.
- [x] The developer workflow doc describes `run --scenario`, and how to enable the end-to-end test.

## Answer

Implemented on branch `ticket-05`.

- `tools/scenario_live.py`: `play(events, hwnd, backend, take_shot, say, ...)` walks a scenario plan: runs of key events go through `pad_live.run_plan` (so every abort releases keys and restores focus), idle time before a shot is waited out, and each `take_shot(label)` result is printed. `wait_for_window(find, timeout=30)` polls until the window exists, else a clear error (nothing sent).
- `dev.py`: `scenario <name>` now plays live (`play_live`); `run --scenario <name>` validates the scenario first (an unknown name fails before building), builds, launches, waits up to 30 s for the window, then plays.
- Live finding: Windows' foreground lock refused `SetForegroundWindow` from this background process ("could not bring the Dolphin window to the front"). `Win32Backend.focus` now falls back to a tap of Alt (it lands on the current foreground window) and retries; that fixed it.
- Tuned `boot-to-character-select` live (shots read): the title logo appears about 85-90 s after boot; `wait:5400 start` lands on the title, a second `start` on the main menu, the main menu ignores input for a few seconds (wait 240 frames before `stick-down`), which needs 12 frames (6 did not move the cursor), then `a a` reaches VS Melee character select. Shots `title`, `main-menu`, `character-select`.
- E2E (Seam 2): `tools/dev_tests/test_e2e.py`, enabled by `SSBM_E2E=1`, also skipped without disc, Dolphin or Windows; uses scenarios `e2e-title` and `e2e-smoke`. Documented in `docs/developer-workflow.md` along with `run --scenario`.
- Tests: `tools/dev_tests/test_scenario_live.py` (fake backend play, abort and shot failure, window-wait timeout, CLI error cases).
