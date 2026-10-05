# Emulator piloting: drive the training build and see the result

Status: ready-for-agent

## Problem Statement

`dev.py run` builds the training build and launches Dolphin on it, and that's where the agent's view stops. Once Dolphin is up, the agent working in a Claude Code session can't see the game or control it. So it can't confirm a change had the visible effect it expected before moving on. Every visual check falls back to me: I play to the right screen, look, and report back. That makes me the bottleneck in every iteration, and the agent ends up building on changes nobody has verified.

## Solution

Give the agent two things on Windows, both through `dev.py`. **Piloting**: send GameCube controller input to the running training build and take screenshots it can read back. **Scenarios**: committed, named input scripts that take the training build from boot to a known game state and take screenshots along the way. Those screenshots can then be checked without spending tokens on a screenshot → look → next-input loop.

The expected workflow is a loop. The agent pilots by hand during a session to reach a state it needs. Sequences that come up again and again get written down as scenarios, so they become cheap and repeatable.

Input is delivered as synthetic keystrokes using my existing Dolphin keyboard bindings. Those keystrokes only reach Dolphin while its window has focus, so taking focus is a visible, permission-gated act. By default the agent asks before doing anything that takes focus, including launching Dolphin. I can waive that for a session.

## User Stories

1. As the agent, I want to send a GameCube button press to the running training build, so that I can navigate menus.
2. As the agent, I want to hold several controls at once (e.g. stick down + B), so that I can perform game actions that need combined inputs.
3. As the agent, I want to give hold durations in frames, so that my inputs line up with how Melee reads input (once per frame at 60 Hz).
4. As the agent, I want a default hold of 3 frames when I don't give a duration, so that simple taps are short to write and still register reliably.
5. As the agent, I want an idle step (`wait:N`), so that I can let menus and transitions finish before the next input.
6. As the agent, I want to write input in GameCube controls (`a`, `start`, `stick-down`, `l`, `dpad-up`), not keyboard keys, so that what I write reads like Melee and survives a rebind.
7. As the agent, I want the controls translated to keys using my current port-1 bindings in Dolphin's pad config, read fresh every time, so that I never have to keep a copy of the mapping in sync.
8. As the agent, I want a clear error when port 1 isn't bound to the keyboard, or a control I used is unbound, so that I don't send input that silently does nothing.
9. As the agent, I want a clear error for a malformed sequence, naming the bad step, so that I can fix it quickly.
10. As the agent, I want a dry-run mode that prints the timed key-event plan without sending anything, so that I can check a sequence before taking focus.
11. As the agent, I want to take a screenshot of the game frame with a label, so that I can read the image and verify the game state.
12. As the agent, I want a screenshot moved to a predictable path and that path printed, so that I can open it straight away.
13. As the agent, I want an unlabelled screenshot to get a timestamp name, so that quick shots don't overwrite each other.
14. As the agent, I want screenshots captured by Dolphin itself, not by grabbing the window, so that they come out right whatever the graphics backend or other windows covering Dolphin.
15. As the agent, I want the training build's Dolphin window found automatically by its title, so that I don't need to know window handles or PIDs.
16. As the agent, I want the tool to bring Dolphin to the front before sending input, so that the keystrokes reach the game.
17. As the developer, I want focus checked before every key event and the sequence aborted loudly if Dolphin has lost focus, so that stray keystrokes never land in my editor or another app.
18. As the developer, I want the window that had focus before restored afterwards, so that I can carry on where I was.
19. As the developer, I want every key the tool pressed released on any abort (lost focus, missing binding, Ctrl+C), so that no key stays held in Dolphin.
20. As the developer, I want the tool to refuse when more than one window matches, so that it never guesses between my own Dolphin and the one `run` launched.
21. As the developer, I want `run` to replace only the Dolphin instance it launched itself, tracking its process, so that a stale build is never left running while any Dolphin I opened myself is left alone.
22. As the agent, I want to run a named scenario against the running training build, so that I can reach a known game state in one command.
23. As the agent, I want `run --scenario <name>` to launch the training build, wait for its window and then play the scenario, so that I can go from a code change to a verified state in one step.
24. As the agent, I want the boot wait to time out with a clear error, so that I never send input into a window that isn't there.
25. As the agent, I want scenario files in plain text with one step per line and `#` comments, so that both I and the developer can read and write them by hand.
26. As the agent, I want scenario files to use exactly the same step grammar as one-off piloting, so that codifying a piloting session is copy and paste.
27. As the agent, I want a `shot <label>` step in scenarios, so that a scenario can capture checkpoints along the way.
28. As the agent, I want `include <other>` in scenarios, so that shared prefixes such as getting from the title screen to character select are written once.
29. As the agent, I want include cycles and missing includes reported as errors before any input is sent, so that a broken scenario fails fast.
30. As the agent, I want a first committed scenario that goes from boot to character select, so that there's a working example and a reusable prefix.
31. As the developer, I want screenshots cleared on each `run`, so that the shots folder only reflects the current build.
32. As the developer, I want the piloting commands to fail with a clear "not supported yet" message on macOS, so that the limitation is obvious and not a crash.
33. As the developer, I want the focus-permission rule written in the agent instructions, so that every session asks before taking focus unless I've waived it.
34. As the developer, I want piloting and scenarios documented in the developer workflow, so that I and future agents know how to use them.
35. As the developer, I want an opt-in end-to-end test against real Dolphin, so that the live parts are verified when I choose, without taking focus during routine checks.

## Implementation Decisions

- **Single entry point.** Everything is new `dev.py` subcommands plus a `run` option, in line with ADR-0002. Commands are stateless: each looks for the Dolphin window afresh. There's no long-lived controller process or server.
- **Commands:**
  - `pad "<sequence>" [--dry-run]` plays a step sequence against the running training build.
  - `shot [label]` takes a screenshot and prints its path.
  - `scenario <name> [--dry-run]` plays a committed scenario against the running training build.
  - `run --scenario <name>` launches the build, waits for the game window (timeout around 30s), then plays the scenario.
- **Step grammar, shared by `pad` and scenario files:** steps are separated by whitespace on the command line and by lines in files. A step is either a set of controls joined by `+` with an optional `:N` hold in frames (default 3), or `wait:N`. Scenario files also allow `shot <label>`, `include <name>` and `#` comments. Frames convert to wall-clock time at 60 fps. Timing is approximate, not frame-exact.
- **Control vocabulary:** `a b x y z start l r`, `stick-up/down/left/right`, `cstick-up/down/left/right` and `dpad-up/down/left/right`. Analog modifier keys aren't exposed in v1.
- **Binding resolution:** the tool reads the `[GCPad1]` section of Dolphin's pad config from Dolphin's user folder. That folder is found from the configured Dolphin (`portable.txt` beside the executable means the `User` folder there, otherwise the standard per-user location). It can be overridden by a new optional `dolphin_user` key in `dev.config.json`, which is also how tests supply a fixture. The device must be the keyboard. Dolphin binding names (e.g. `COMMA`, `PERIOD`, backtick-quoted names) map to scancodes.
- **Input delivery:** Win32 `SendInput` with scancodes, because Dolphin reads the keyboard through DirectInput and ignores posted window messages. Before each key event the tool checks that the Dolphin window is in the foreground. If it isn't, it releases every held key and exits non-zero. On success, and on every failure path, the window that had focus before is restored and held keys are released.
- **Window identification:** found by title among top-level windows belonging to the Dolphin process `run` launched, falling back to title matching. Zero or several matches is an error. The exact title Dolphin uses when booting a bare DOL is still to be confirmed at implementation time (launching to check takes focus, so ask first).
- **Instance tracking:** `run` records the PID of the Dolphin it launched in the existing dev state file. On the next `run` it terminates that process (if it's still a Dolphin process) before launching. It never touches any other Dolphin process.
- **Screenshots:** sent through Dolphin's own screenshot hotkey (default F9; read from Dolphin's hotkey config if rebound). The tool waits for a new file in Dolphin's screenshots folder, moves it into the training build's shots folder (under the gitignored build output) as `<label>.png`, and prints the path. The shots folder is cleared at each `run`.
- **Scenarios:** committed plain-text files in a scenarios folder under `tools/`, named by file stem. `include` is resolved and expanded before any input is sent, and cycles and missing files are errors. The first scenario takes the game from boot to character select.
- **Dry run:** for `pad` and `scenario`, it resolves bindings and includes and prints the ordered, timed key-down/key-up plan (and shot steps). It sends nothing and needs no running Dolphin.
- **Platform:** Windows only. On macOS, `pad`, `shot`, `scenario` and `run --scenario` exit with a clear "not supported yet" message. Plain `run` behaves as it does today.
- **Focus permission is agent behaviour, not code.** The tool has no prompt or flag. The agent instructions (CLAUDE.md and the developer workflow doc) say: ask the user before any command that takes focus, including `run`, unless the user has waived that for the current session.
- **Vocabulary:** use the glossary terms **piloting** and **scenario** in commands, help text and docs.

## Testing Decisions

- Tests check external behaviour only: they run the real `dev.py` command line and observe exit codes, printed output and files on disk, as `tools/dev_tests/` already does.
- **Seam 1: CLI without a live Dolphin (routine).** `pad --dry-run` and `scenario --dry-run`, with `dolphin_user` pointed at a fixture pad config, cover: the sequence grammar and error messages, default holds, combined controls, frame-to-time conversion, binding mapping including special key names, unbound control and non-keyboard device errors, `include` expansion, cycle and missing-include errors, and the macOS "not supported" message where it can be reached.
- **Seam 2: live end-to-end (opt-in).** A single test boots the training build with `run --scenario` using a small scenario that reaches the title screen and takes a shot, then checks that the PNG exists at the printed path. It also covers PID replacement by running twice. It's skipped unless an explicit environment variable is set (it takes focus) and also skips without a disc or Dolphin. It isn't part of `dev.py check`.
- Prior art: `tools/dev_tests/support.py` (CLI driver, `requires_disc`), and `test_run.py` (swaps Dolphin through `dev.config.json` and restores the config afterwards).

## Out of Scope

- macOS support for piloting (Accessibility permissions and CGEvent).
- Frame-exact input, pause and frame advance, and Dolphin TAS movie (`.dtm`) playback. These are the expected route if determinism becomes necessary.
- Virtual gamepad drivers (ViGEm/vJoy).
- Input scripted from inside the game via `TRAINING_BUILD` hooks.
- Save-state-based scenario starting points (save states go stale as code changes).
- Analog stick magnitudes and modifiers, and ports other than port 1.
- Automated image comparison of shots. The agent (or the developer) inspects them.
- Running piloting as part of `dev.py check`.

## Further Notes

- Window capture (PrintWindow/BitBlt) was rejected because D3D/Vulkan swapchains can come back black.
- Turning on Dolphin's Background Input doesn't remove the need for focus: synthetic keystrokes still go to whichever app has focus.
- Suggested slices: (1) window lookup, focus guard and restore, PID-tracked replacement in `run`; (2) `pad` with the grammar, binding mapping and dry run; (3) `shot`; (4) scenarios, `include`, `scenario` and `run --scenario`, plus the first scenario; (5) docs and the focus-permission rule in CLAUDE.md.
