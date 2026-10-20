# 01: Characters in the drill_start rep log

**What to build:** In the authoring build, the [rep log](../../../docs/glossary.md#glossary_rep_log)'s `drill_start` line names the player's and the opponent's characters. A scenario can then check which fighters a drill started with without reading a shot. Player builds are unchanged.

**Blocked by:** None (can start immediately)

**Status:** ready-for-agent

- [x] `drill_start` carries the player's and the opponent's character kinds
- [x] `event-change-characters` shows Ness on the first `drill_start` and Luigi on the second and third
- [x] Nothing new is logged or compiled into player builds
- [x] The developer workflow's rep log section lists the new fields
- [x] `dev.py check` passes
