# 04: Scenario files: `scenario --dry-run`

**What to build:** Committed, plain-text scenarios that use the same step grammar as `pad`, plus `shot <label>`, `include <name>` and `#` comments. `dev.py scenario <name> --dry-run` expands a scenario and prints its full plan. Ships the first scenario (boot to character select) as a reusable prefix. See the spec (`../spec.md`).

**Blocked by:** 01 (`pad --dry-run`).

**Status:** resolved

- [x] Scenarios live in a scenarios folder under `tools/`, named by file stem, with one step per line. Blank lines and `#` comments are ignored.
- [x] `include <name>` is expanded in place before any input is sent. Include cycles and missing scenarios are clear errors that name the chain.
- [x] `shot <label>` steps appear in the dry-run plan at their position.
- [x] `dev.py scenario <name> --dry-run` prints the expanded, timed plan without needing a running Dolphin. An unknown scenario name is a clear error listing the available ones.
- [x] On macOS, `scenario` exits with a clear "not supported yet" message.
- [x] A first scenario that goes from boot to character select is committed and passes dry run (its waits get tuned live in ticket 05).
- [x] Tests in `tools/dev_tests/` drive the real CLI with fixture scenarios: comments, includes, cycles, missing includes and shot steps (Seam 1).
- [x] The developer workflow doc describes scenario files and codifying piloting into a scenario.

## Answer

Implemented on branch `ticket-04`.

- `tools/scenario.py` (pure logic, imports `tools.pad`): `expand(name, folder=SCENARIOS_DIR) -> [Step | Shot]`, a flat list with every include expanded in place (`Step` is pad's namedtuple; `Shot(label)` is a namedtuple). `build_plan(items, bindings) -> [KeyEvent | ShotEvent(ms, label)]` (pad's `KeyEvent`; times come from frame counts so rounding does not drift; a shot sits at the time the preceding steps end). `format_plan(events)` uses pad's line format plus `<ms> ms  shot  <label>` for shots. Also `available(folder)`, `ScenarioError`, `SCENARIOS_DIR` (`tools/scenarios`, files `<name>.txt`).
- For ticket 05: walk the events from `build_plan` in order, sending `down`/`up` key events at their `ms` offsets and taking the screenshot named `label` at each `ShotEvent`. Or walk `expand()` items directly (Step -> hold, Shot -> screenshot) with its own timing.
- Errors: unknown scenario lists the available ones; cycle reads `a -> b -> a`; missing include reads `a -> b -> nope`; a bad line is `name:lineno: ...`. A line may hold several whitespace-separated steps; `shot` and `include` take exactly one argument.
- `dev.py`: `cmd_scenario` uses `require_piloting_platform()`; without `--dry-run` it errors "not implemented yet", for ticket 05 to replace. Test sandboxes must copy `tools/scenario.py` too; fixtures are written into the sandbox's `tools/scenarios`, so no folder override was needed.
- First scenario `tools/scenarios/boot-to-character-select.txt` (shots `title`, `character-select`); its waits are a best guess, flagged in a comment, to be tuned in ticket 05.
- Tests: `tools/dev_tests/test_scenario.py`. Docs: "Scenarios" section in `docs/developer-workflow.md`.
