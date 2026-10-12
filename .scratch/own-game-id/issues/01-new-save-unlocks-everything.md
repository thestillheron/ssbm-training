# 01: New save opens every character and stage

**What to build:** when the training build creates a save (Melee's boot-time "no save" prompt, answered yes), every character and every stage is unlocked, and the modes already reachable stay so. Trophies and other collectibles are left alone. Melee's own create-save prompt is kept, so the player can still decline. This lands while the build still runs as GALE01, so it can be checked against a blank card before the game ID changes ([spec](../spec.md), ADR-0006).

**Blocked by:** None (can start immediately)

**Status:** resolved

- [x] Booting the training build against a blank card and creating a save leaves every character selectable on the character select (e.g. Marth, Falco, Mewtwo, Mr. Game & Watch).
- [x] Every stage, including Final Destination and Dream Land, is selectable on the Custom drill's stage select.
- [x] Trophy and other collectible data are unchanged from a vanilla new save.
- [x] Declining the create-save prompt still plays without saving, as in vanilla.
- [x] The matching build is unaffected (`dev.py check`).

## Answer

Resolved on branch `worktree-agent-a0848b29dd252c518` (commit "New save opens every character and stage"). `gm_Scene_MemCard_OnFrame` (case 6, just before the create-save write) calls `training_new_save_unlock_all()` under `TRAINING_BUILD` (`src/training/new_save.c`), which sets every character and stage bit; the feature bitmask and trophies are not touched directly. Checked live against a blank card with the new scenarios `new-save-character-select` and `new-save-stage-select` (red before, green after); `dev.py check` passes. Side effect: the game then shows its own unlock notices once on first main-menu entry (random stage select, sound test, all characters, all stages, and a Kirby Hat trophy), which `boot-new-save-to-main-menu` clears. The decline path is untouched code.
