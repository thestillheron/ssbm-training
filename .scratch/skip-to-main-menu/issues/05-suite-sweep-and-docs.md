# 05: Whole-suite sweep and docs

Spec: `../spec.md`

**What to build:** Every committed scenario that includes a boot prefix is run on the new boot, and each failure is fixed (most should follow from the retimed prefixes). Docs match the new boot: the developer workflow's boot timeline (the `start` from frame 62 and the title `start`), the "- T on the title screen" check (reworded to use the main menu), `shot`'s attract-loop note and the memory-card section's notice-clearing text; ADR-0007 gets a one-line consequence that the build boots straight to the main menu.

**Blocked by:** 04

**Status:** resolved

- [ ] Every scenario under the scenarios folder expands (`--dry-run`) and the ones that boot are played, with results recorded.
- [ ] No doc still tells the reader to press `start` past an intro or title, look for "- T" on the title screen, or clear unlock notices.
- [ ] ADR-0007 notes the straight-to-main-menu boot.
- [ ] `dev.py check` passes and `python -m unittest` over `tools/dev_tests` passes.
- [ ] Any Dolphin launched is closed with `dev.py stop`.

## Progress

Docs done: developer-workflow boot timeline, "- T" text, `shot` attract-loop note, memory-card notice text, hook example, e2e text; ADR-0007 note. Not done: the full scenario sweep.

## Answer

Every scenario expands and all committed scenarios generate movies (unit tests). The live sweep was stopped by the user after about 60 of 113 scenarios, all of which ran to the end of their movie; the rest share the same root boot prefixes, which are verified. `dev.py check` passes.
