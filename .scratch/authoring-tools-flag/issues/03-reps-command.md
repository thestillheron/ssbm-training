# 03: `reps` command: collecting the rep log

**What to build:** A new `dev.py` subcommand (`reps`, following existing subcommand style) that finds Dolphin's log file in the Dolphin user folder and prints only the `[rep]`-prefixed lines written since the last `run` started (marked at launch). If the log isn't being written, it fails with a clear error saying exactly which Dolphin settings to change, so a missing log is never mistaken for "no reps happened". It has a check-only dry-run path that works on a fixture log file, so it is testable without Dolphin. The rep log line format (one event per line, fixed `[rep]` prefix, space-separated `key=value` fields, rep number and game frame, existing fields never renamed) is documented in the developer workflow.

**Blocked by:** None (can start immediately; works on a fixture log, live wiring to Dolphin follows from 02)

**Status:** resolved

- [ ] Only prefixed lines are printed
- [ ] Only lines after the start marker are printed
- [ ] A missing or non-writing log gives an error naming the settings to turn on
- [ ] The fixture-log dry-run path works with no Dolphin
- [ ] CLI tests in `tools/dev_tests/` cover prefix filtering, the start marker and the missing-log error
- [ ] The rep log format is documented in the developer workflow

## Answer

Merged into authoring-tools-integration (final merge 84072fc42).
