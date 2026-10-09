# 01: Custom drill event with defaults

**What to build:** A "Custom drill" event at the end of the Event Match list. Choosing it goes through a new stage select step, then the character select, then a match running a session-scoped custom drill record (same record type as curriculum drills). Defaults: landing point at the main floor's middle (found by a floor ray-cast at the stage centre), one drop height (the curriculum's base height), player start at the curriculum's default offset, all four tech options, vanish off, 10 reps, no pass mark, player reset on, opponent facing the player. It works on any stage; if the stored layout has no floor on a new stage it falls back to the defaults. The rep log writes `drill_start` with the custom settings.

**Blocked by:** None (can start immediately).

**Status:** resolved

- [x] Custom drill appears as the last event and opens a stage select, then character select
- [x] The drill starts on the chosen stage and runs 10 reps with the defaults
- [x] A new stage with no floor under the stored layout falls back to the defaults
- [x] `drill_start` in the rep log carries the custom settings
- [x] Scenario with shots covers the entry path on at least two stages
