# 02: Live `pad`: window lookup, focus guard, `run` replaces its own instance

**What to build:** `dev.py pad "<sequence>"` (without `--dry-run`) drives the running training build. It brings Dolphin to the front, sends the planned keystrokes with `SendInput` scancodes, and hands focus back afterwards. `run` keeps track of the Dolphin it launched and replaces only that one on the next `run`, so there's never a stale build running alongside or more than one window to choose from. See the spec (`../spec.md`).

**Blocked by:** 01 (`pad --dry-run`).

**Status:** resolved

- [x] First, confirm the exact window title Dolphin uses when booting a bare DOL. Launching Dolphin takes focus, so **ask the user before launching**. Record the finding in this ticket's Comments.
- [x] The window is found by title among windows of the Dolphin process that `run` launched, falling back to title matching. Zero or several matches is a clear error, and nothing is sent.
- [x] Before each key event the tool checks that Dolphin is the foreground window. If it isn't, it releases every held key and exits non-zero with a "focus lost" message.
- [x] Every held key is released, and the previously focused window restored, on success and on every failure path, including Ctrl+C.
- [x] `run` records the PID of the Dolphin it launches in the dev state file. On the next `run` it terminates that process (only if it's still a Dolphin process) before launching. Other Dolphin processes are never touched.
- [x] Verified by hand against a real training build: a `start` press on the title screen moves the game on.
- [x] The focus-permission rule is added to `CLAUDE.md` and the developer workflow doc: ask the user before any command that takes focus, including `run`, unless the user has waived that for the current session.

## Comments

Implemented without launching Dolphin or sending any keystrokes (no permission from the user was available). Code: `tools/pad_live.py` (guard/release logic, injectable backend, unit-tested with a fake), `tools/win_pilot.py` (ctypes: window enumeration, SendInput, foreground, process image/terminate; untested against a real window), `dev.py` (`stop_tracked_dolphin`, `dolphin_pid` in `build/dev_state.json`, live `cmd_pad`). Tests: `tools/dev_tests/test_pad_live.py`.

The "PID tracking" test uses fake image/terminate functions, not a real `run` (that needs a disc image and launches Dolphin).

Title-matching assumption: Dolphin's window title starts with "Dolphin" (e.g. `Dolphin 2503 | JIT64 DC | ...`). The pattern is a case-insensitive regex, default `^Dolphin`, set by `DEFAULT_TITLE_PATTERN` in `tools/win_pilot.py` or per machine by `"dolphin_window_title"` in `dev.config.json`. Windows of the `run`-launched PID matching the pattern are preferred; otherwise any matching window is used. Several matches is an error.

**The user must verify by hand:**

1. `python dev.py run`, then read the actual title of the Dolphin window while the bare DOL boots (and on the title screen). Check it matches `^Dolphin`; if not, set `dolphin_window_title` or change the default, and record the real title here. Check there is exactly one matching window (Dolphin's main frame and render window both being visible with "Dolphin" titles would be reported as an ambiguity error).
2. With the title screen showing and GameCube port 1 bound to the keyboard, run `python dev.py pad "start"`. Expect: Dolphin comes to the front, the game advances past the title screen, and the previous window regains focus.
3. Focus-lost path: run a long sequence (e.g. `"wait:300 a"`) and click another window mid-way. Expect a non-zero exit with "focus lost", and no stuck key in the game.
4. Ctrl+C mid-sequence: no stuck key, focus restored.
5. `python dev.py run` a second time: the first Dolphin is closed and exactly one new one remains; a Dolphin started by hand (not via `run`) is left alone.

### Orchestrator live check (user waived focus for the session)
- Bare-DOL window title: `Dolphin 2609 | JIT64 SC | Direct3D 11 | HLE | Super Smash Bros. Melee (GALE01)`. The same Dolphin process also owns a second top-level window titled just `Dolphin 2609`, so the old `^Dolphin` pattern matched twice. Default pattern is now `^Dolphin .*\|`.
- `pad "wait:30 start wait:120"` ran with exit 0 against the `run`-launched Dolphin while the user's own Dolphin (other PID) was also open. Visual confirmation that `start` advanced the game waits on `shot` (ticket 03).

### Ticket 03 live check: `start` advances the game
With the `run`-launched Dolphin, shots confirmed: `pad "start wait:90"` during an attract-mode demo cut back to the title screen (before: demo; after: title with the "- T" marker); a second `pad "start wait:120"` on the title screen reached the main menu (1-P Mode, VS. Mode, ...). Both exited 0. Other Dolphin processes were not touched.
