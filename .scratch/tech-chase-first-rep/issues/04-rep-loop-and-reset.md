# Rep loop: detect end, pause, reset

Status: resolved
Blocked by: 03
Spec: ../spec.md (stories 5, 8, 10)

## What to build

Complete the state machine: *landed* → *ended* (opponent has left knockdown/tech states and is actionable) → ~1 s pause → *reset* → next rep, endlessly. The player's hits and grabs behave normally. The next rep must start cleanly if the opponent was knocked away, grabbed, or otherwise displaced (including off stage); player is returned to start state at every reset.

## Acceptance criteria

- [ ] Reps loop indefinitely with ~1 s between actionable and next drop.
- [ ] Resets work after the opponent was hit away, grabbed, or killed/respawned.
- [ ] Player is grounded, standing, facing the opponent, 0% at every rep start.
