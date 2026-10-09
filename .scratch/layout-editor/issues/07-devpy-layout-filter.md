# 07: dev.py layout filter for the rep log command

**What to build:** dev.py's rep log command gains a filter that shows only layout lines, so the printed layout text can be seen after a session without digging through Dolphin's log.

**Blocked by:** 06 (for the line format)

**Status:** resolved

- [x] Rep log command filter shows only layout lines
- [x] CLI test on a fixture log

## Answer

`python dev.py reps --layout` prints only the Print layout lines (`[rep] layout: ...`, not `event=layout`), with the same `--log`/`--since` spans as `reps`; none gives a stderr note "no [rep] layout: lines". Filter is `reps.layout_lines` (`tools/reps.py`). CLI tests in `tools/dev_tests/test_reps.py` on a fixture log; documented under Rep log in `docs/developer-workflow.md`.
