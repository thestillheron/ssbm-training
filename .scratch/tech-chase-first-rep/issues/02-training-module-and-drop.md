# Training module: silenced opponent that drops in tumble each rep

Status: resolved
Blocked by: 01
Spec: ../spec.md (stories 1, 2, 6, 7, 9, 11, 13, 14)

## What to build

The training module, training build only and Training Mode only. One guarded hook in the Training scene on-enter creates a per-frame think object. One guarded hook between the CPU tick and the read of CPU input into the fighter input replaces the opponent's input every frame (neutral outside a rep), silencing the AI (ADR-0004). The think object runs the first states of the rep state machine: *reset* (player standing at the fixed start position, facing the opponent, 0%; opponent percent 0, faces player, velocities and knockback cleared) then *drop* (opponent enters tumble via the game's tumble-entry function from rest at the fixed drop point above FD centre). Placement sets position then resyncs collision data and environment collision box. Hardcoded layout.

## Acceptance criteria

- [ ] In Training Mode on FD the opponent appears in tumble above centre stage with no sideways drift and no AI actions, for any character pair.
- [ ] Without a tech press it lands as a missed tech (expected at this stage).
- [ ] Upstream changes are single guarded hook lines only.
- [ ] `dev.py check` passes: matching build byte-identical, training build links.
