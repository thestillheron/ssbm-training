# 03: Declined save and no-card boots reach the main menu

Spec: `../spec.md`

**What to build:** The boot-time prompts are kept: the create-save prompt (the player can decline) and, with no card, "Continue without saving or loading Game Data?". After either, the game lands on the main menu with nothing queued behind it, skipping the intro and title as in ticket 01. `new-save-declined-history` and `drill-history-saved-nocard` are retimed to the new flow, and a `--no-card` boot scenario is added if none exists.

**Blocked by:** 01, 02

**Status:** resolved

- [ ] Declining the create-save prompt plays on to the main menu with no notice and the cursor on RTC 101.
- [ ] With `run --no-card` the "Continue without saving?" prompt still shows, and answering it reaches the main menu.
- [ ] `new-save-declined-history` and `drill-history-saved-nocard` pass.
- [ ] `dev.py check` passes.
- [ ] Any Dolphin launched is closed with `dev.py stop`.

## Answer

Declined save: `new-save-declined-history` passes live (menu, then `Test: ten reps`, `history_record attempts=1`). No card: `drill-history-saved-nocard --no-card` passes live; it needs two `a` presses (the notice, then "Continue without saving?"), which the old `start` presses used to provide.
