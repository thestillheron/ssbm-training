# 03: Other ways of changing characters on the custom drill

**What to build:** Every way of changing characters between custom drill matches loads the next match, with or without an edited layout. The routes are: change the player, change both characters, pick a different stage after the change, and back out of the stage select with B before picking. If the edited layout leaves the player no floor next to the new opponent, the drill shows its [invalid layout](../../../docs/glossary.md#glossary_invalid_layout) notice instead of crashing.

**Blocked by:** 02

**Status:** ready-for-agent

- [ ] A scenario for a changed player on the custom drill: the drill starts with the new player
- [ ] A scenario for both characters changed: the drill starts with both new fighters
- [ ] A scenario for a different stage after the change: the drill starts on that stage
- [ ] A scenario that backs out of the stage select with B to the character select, changes the opponent and picks a stage: the drill starts
- [ ] Any route that still crashes is fixed, and its cause is noted under `## Comments`
- [ ] The tickets 01 and 02 scenarios still pass
- [ ] `dev.py check` passes
