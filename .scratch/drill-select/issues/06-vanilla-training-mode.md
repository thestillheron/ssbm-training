# 06: Training Mode back to vanilla, scenarios migrated

**What to build:** Drills run only as events. Training Mode loses the drill hook, the CPU takeover, the score and the [drill chooser](../../../docs/glossary.md#glossary_drill_chooser), and behaves exactly as vanilla Training Mode, with its own pause menu and CPU settings working again. Every scenario that boots to Training to run a drill (tech-vanish-shots and the others) moves to Event Match entry and chooses its drill from the list instead of with the chooser.

Remove the code that only served hosting drills in Training Mode: the chooser, the `no_drill_for_stage` reporting, fitting the drill to the stage in play, and the Training pause-flag workarounds. Remove the drill chooser entry from the glossary, and mark the drill-select item done in priorities.

**Blocked by:** 02, 05

**Status:** ready-for-agent

- [ ] In Training Mode no drill runs, and the rep log has no rep events (scenario)
- [ ] Training Mode's own pause menu and CPU settings work as in vanilla (shot)
- [ ] Every drill scenario goes through Event Match entry, and none use the drill chooser
- [ ] The drill chooser and the stage-fitting code are gone, and the glossary no longer defines the drill chooser
- [ ] The full spec scenario passes: list → drill 2 → pause → restart → event list → drill 1 → pause → Change characters → start → L+R+A+Start → CSS
- [ ] `dev.py check` passes
