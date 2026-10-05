# 01: `pad --dry-run`: sequence grammar and binding mapping

**What to build:** `dev.py pad "<sequence>" --dry-run` resolves a piloting sequence written in GameCube controls into the ordered, timed key-down/key-up plan it would send, and prints it without touching Dolphin. This is the first piloting tracer bullet: the grammar and the mapping from controls to keys, verifiable without a live game. See the spec (`../spec.md`) for the grammar and vocabulary.

**Blocked by:** None (can start immediately).

**Status:** resolved

- [x] Steps are separated by whitespace. A step is controls joined by `+` with an optional `:N` hold in frames (default 3), or `wait:N`. Frames convert to wall-clock time at 60 fps.
- [x] Control vocabulary: `a b x y z start l r`, `stick-*`, `cstick-*`, `dpad-*` (up/down/left/right).
- [x] Bindings are read fresh from `[GCPad1]` of Dolphin's pad config in Dolphin's user folder: `portable.txt` beside the configured Dolphin means its `User` folder, otherwise the standard per-user location. The new optional `dolphin_user` key in `dev.config.json` overrides that.
- [x] Dolphin binding names (plain letters, `COMMA`, `PERIOD`, backtick-quoted names) map to keyboard scancodes.
- [x] Clear non-zero errors for: a malformed step (naming it), an unknown control, an unbound control, a port-1 device that isn't the keyboard, and a missing pad config.
- [x] On macOS, `pad` exits with a clear "not supported yet" message.
- [x] Tests in `tools/dev_tests/` drive the real CLI with a fixture pad config through `dolphin_user` and check the printed plan and errors (Seam 1).
- [x] The developer workflow doc gains a short piloting section describing `pad` and the step grammar, using the glossary terms.

## Answer

Implemented on branch `ticket-01`.

- `tools/pad.py` holds all the logic and has no Win32 or Dolphin dependency, so later tickets add the live sender elsewhere. API: `parse_sequence(text) -> [Step(text, controls, frames)]` (`controls == ()` means `wait`), `parse_step`, `pad_config_path(user_dir)`, `load_bindings(ini_path) -> {control: Key(name, scancode, extended)}` (fresh read of `[GCPad1]`; raises on missing file or non-keyboard device; unbound controls are simply absent), `resolve(control, bindings)` (raises "unbound control"), `build_plan(steps, bindings) -> [KeyEvent(ms, action, control, key)]`, `format_plan`, `key_from_name` (Dolphin names, backticks, aliases; DirectInput set-1 scancodes, arrow keys flagged `extended`), `frames_to_ms`. Errors are `pad.PadError`. Constants: `CONTROLS`, `DOLPHIN_KEYS`, `FPS`, `DEFAULT_HOLD_FRAMES`.
- `dev.py`: `cmd_pad`, `dolphin_user_dir()` (config `dolphin_user`, else portable.txt `User`, else per-user dir), `require_piloting_platform()` (macOS "not supported yet"; reuse in `shot`/`scenario`). `pad` without `--dry-run` currently errors "not implemented yet" (ticket 02). `dev.py` imports `tools.pad` lazily inside `cmd_pad`.
- Plan output: one line per event, `<ms> ms  down|up  <control>  <DolphinKeyName>[ (extended)]  0xSC`. Steps run back to back; no gap is inserted between consecutive presses of the same key (use `wait:N`).
- Tests: `tools/dev_tests/test_pad.py` runs the real CLI in a sandbox copy (`dev.py` plus `tools/pad.py`) with `dolphin_user` in the sandbox `dev.config.json` pointing at a fixture `Config/GCPadNew.ini`. New tools modules needed by later tickets must also be copied into that sandbox. The macOS test is skipped on Windows.
- Docs: new "Piloting" section and `dolphin_user` key in `docs/developer-workflow.md`.
