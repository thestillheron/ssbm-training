# 02: Spike: does OSReport reach Dolphin's log?

**What to build:** Verify the spec's unverified assumption: that OSReport from a bare booted training `main.dol` reaches Dolphin's log. Add a one-line authoring-only OSReport to the training build and launch it from `run` with per-launch Dolphin config overrides (file logging and the OS report log type on). If Dolphin needs the game's symbol map to recognise the print function, have `run` place the build's map where Dolphin looks for it. Record the working route, or, if none works, the exact Dolphin settings to turn on. If no route works at all, record in the spec the fallback of drawing the latest rep log lines as on-screen text read from shots, and reshape tickets 03 to 05.

**Blocked by:** 01: Authoring flag

**Status:** resolved

**Note:** the live check takes Dolphin window focus, so ask the user before running it (see CLAUDE.md focus permission).

- [ ] A live run shows the authoring OSReport line in Dolphin's log file, or the failure is established
- [ ] Per-launch overrides are used if Dolphin honours them for logger settings; otherwise the exact settings to enable are documented
- [ ] The finding and chosen route are recorded in the spec and the developer workflow
- [ ] The non-authoring training build contains no such print

## Answer

Merged into authoring-tools-integration (final merge 84072fc42).
