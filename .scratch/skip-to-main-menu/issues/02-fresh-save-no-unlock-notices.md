# 02: Fresh save shows no unlock notices

Spec: `../spec.md`

**What to build:** On a card with no save for this build, answering the create-save prompt with Yes leads straight to a usable main menu: the unlock notices Melee queues for what the new save holds (random stage select, sound test, all characters, all stages, the trophy) are suppressed where the save is created, so they never show, on first or later launches. What the save holds does not change (every character and stage unlocked, ADR-0006). Any other notice found queued on first main-menu entry is handled the same way. `boot-new-save-to-main-menu` loses its intro/title `start` presses and its eight notice-dismissing `a` presses.

**Blocked by:** 01

**Status:** resolved

- [ ] Against a blank card, `boot-new-save-to-main-menu` reaches the main menu with no notice on screen, and an `a` right after it is ready opens RTC 101.
- [ ] A second launch against the card that run saved (`run --keep-card`) shows no notices either.
- [ ] `new-save-character-select` and `new-save-stage-select` still pass: every character and stage is selectable.
- [ ] Trophy and collectible data are as a new save had them before this change.
- [ ] `dev.py check` passes.
- [ ] Any Dolphin launched is closed with `dev.py stop`.

## Answer

Fresh save: the queued unlock notices and trophy flags are cleared right after the unlock checks, on every boot, so nothing is ever queued for the menu. On a card with only a vanilla save: prompt at frame 100, `a` at 160 answers it, the created message ignores presses until about frame 300 so `boot-new-save-to-main-menu` presses `a` at 335, and the menu reads input about 65 frames later. `new-save-character-select` and `new-save-stage-select` pass live; a `--keep-card` second launch shows no notices. Tail of the prefix is 500 frames: Dolphin writes the save to the card in real time, and a 200-frame tail left no save. Found on the way: calling `Toy_SetUnlockState` at boot corrupted the saved card (crash on the next boot), so trophies are not recorded.
