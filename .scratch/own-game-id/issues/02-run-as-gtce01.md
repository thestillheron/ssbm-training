# 02: Training build runs as GTCE01

**What to build:** the training build runs under its own game ID, `GTCE01` (ADR-0006), so Melee and Dolphin treat it as a different game: every save it makes goes to its own file and the player's vanilla Melee save is never read or written. The save's comment reads like "Melee Tech Chase Training" so it can be told apart in Dolphin's memory card manager. The matching build and the original disc files are untouched. Scenario movies keep booting straight to the main menu: the memory card snapshot gains a `GTCE01` save with everything unlocked (from 01), and `dev.py` says how it gets there ([spec](../spec.md)).

**Blocked by:** 01 (New save opens every character and stage)

**Status:** resolved

- [x] Dolphin shows the training build as `GTCE01`; the matching build and `orig/` still say GALE01.
- [x] Dolphin's memory card manager lists the training build's save separately from vanilla Melee's, with the new comment.
- [x] A card holding a vanilla GALE01 save comes through a training-build session that saves with that save byte-identical.
- [x] Every existing scenario passes against the snapshot without stopping at the create-save prompt.
- [x] `docs/glossary.md` (memory card snapshot) and `docs/developer-workflow.md` describe the new ID and how the snapshot gets its `GTCE01` save.

## Answer

Resolved on branch `worktree-agent-a6f58170094d95eba` (commit "Training build runs as GTCE01"). `assemble_game` in `dev.py` writes `GTCE01` into a copy of `sys/boot.bin` in the game folder (the movie header's game ID matches); `orig/` and the matching build still say GALE01. Under `TRAINING_BUILD` the save comment is "Melee Tech Chase Training" (`lb_8001C658`). When the snapshot is a memory card with no `GTCE01` save, `run --scenario` and `snapshot-card` play `boot-new-save-to-main-menu` against it in the training build and keep the card that run saved (`ensure_training_save`); the vanilla save on the card is carried along untouched. Side effect: against a GTCE01 save made by Melee the title screen takes one more `start` than against the vanilla save, so `boot-to-main-menu` presses `start` three times (main menu at frame 194, was 145); other scenarios only wait relative to it. Docs: `docs/developer-workflow.md`, glossary entry for the memory card snapshot.
