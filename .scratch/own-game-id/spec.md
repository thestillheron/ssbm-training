# Own game ID: a save of its own

Status: done

## Problem Statement

The training build runs as GALE01, so Melee and Dolphin treat it as vanilla Melee. Melee finds its save by the disc's game code and company plus the fixed file name `SuperSmashBros0110290334` (`lbcardgame.c:121`, matched in `lbcardnew.c:745-761`). Dolphin files saves by the same ID, both in GCI folder mode (one `.gci` per game and file name) and in raw card mode (one shared card image). Every normal save point in the training build therefore writes the player's real Melee save, and drill [history](../drill-history/spec.md) would too.

## Solution

The training build gets a game ID of its own, `GTCE01` ([ADR-0006](../../docs/adr/0006-training-build-has-its-own-game-id.md)). Every save it makes goes to a separate file, and a new save opens every character and stage.

## Implementation Decisions

- **Game ID.** `GTCE01` written into the disc header (`sys/boot.bin`) of the game folder `dev.py` assembles for the training build only. The matching build and `orig/` are untouched. Melee reads the code back from the disc header, so its card code finds and creates files under the new ID with no code change.
- **Save comment.** The card file's comment (`lb_8001C658`) reads like "Melee Tech Chase Training" under `TRAINING_BUILD`, so the save can be told apart from vanilla Melee's in Dolphin's card manager and the GameCube memory card screen. Banner and icon unchanged.
- **New-save unlocks.** When the training build creates its save (the "no save" path in `gm_Scene_MemCard_OnFrame`), every character and every stage is unlocked, and modes already reachable stay so. Trophies and other collectibles are left alone.
- **Boot prompt kept.** Melee's own "create a save?" prompt stays, so the player can decline and play without saving.
- **Scenario card snapshot.** The [memory card snapshot](../../docs/glossary.md#glossary_memory_card_snapshot) gains a `GTCE01` save, so scenario movies still boot straight to the main menu without the create-save prompt. `dev.py snapshot-card` and its docs cover how that save gets there.
- **ADR-0005** is amended by ADR-0006: the build no longer shares the player's save.

## Testing Decisions

- A scenario from power-on against the snapshot reaches the main menu as before, and the drills still run (the existing scenarios, unchanged, are the check).
- A run against a card with no `GTCE01` save shows the create-save prompt; after creating it, a locked character (e.g. Marth) and Final Destination are selectable.
- A card holding a vanilla GALE01 save: after a training-build session that saves, the GALE01 file is byte-identical to before.

## Out of Scope

- A banner, icon or Dolphin game-list title of our own.
- Migrating anything from the player's vanilla save.
