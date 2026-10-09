# 06: Training Mode back to vanilla, scenarios migrated

**What to build:** Drills run only as events. Training Mode loses the drill hook, the CPU takeover, the score and the [drill chooser](../../../docs/glossary.md#glossary_drill_chooser), and behaves exactly as vanilla Training Mode, with its own pause menu and CPU settings working again. Every scenario that boots to Training to run a drill (tech-vanish-shots and the others) moves to Event Match entry and chooses its drill from the list instead of with the chooser.

Remove the code that only served hosting drills in Training Mode: the chooser, the `no_drill_for_stage` reporting, fitting the drill to the stage in play, and the Training pause-flag workarounds. Remove the drill chooser entry from the glossary, and mark the drill-select item done in priorities.

**Blocked by:** 02, 05

**Status:** done

- [x] In Training Mode no drill runs, and the rep log has no rep events (scenario)
- [x] Training Mode's own pause menu and CPU settings work as in vanilla (shot)
- [x] Every drill scenario goes through Event Match entry, and none use the drill chooser
- [x] The drill chooser and the stage-fitting code are gone, and the glossary no longer defines the drill chooser
- [x] The full spec scenario passes: list → drill 2 → pause → restart → event list → drill 1 → pause → Change characters → start → L+R+A+Start → CSS
- [x] `dev.py check` passes

## Comments

- Training Mode is vanilla. Removed: the `gm_Scene_Training_OnEnter` hook in `gmvs.c` (the only thing starting the rep loop in Training), the D-pad-right drill chooser, `no_drill_for_stage` (logging, notice and the stage check in the Reset state), `training_drill_fits_stage`/`fit_to_stage`/`choose_next`/`stage_name`, the Training pause flag check and the human-driven CPU workaround in `think`, and the notice timeout that only the chooser list used. The `Fighter_procCpu` hook stays: it only acts on a frame the rep loop ran, which now happens only in a drill event.
- `--drill N` / `TRAINING_DRILL_INDEX` is gone (dev.py, configure.py, dev_tests, docs): a drill is chosen by moving down the Event Match list, and each drill brings its stage. Removed scenarios: `drill-chooser`, `drill-wrong-stage`, `boot-to-training-battlefield`.
- Migrated to `boot-to-event` (first drill via Event Match): the `tech-chase-*`, `tech-option-*`, `rep-*`, `rep-outcome-*` families and `drill-end-no-input`. `drill-heights` (drill 2), new `drill-heights-range` (drill 3), `drill-platform-layout` (drill 4, Battlefield), `drill-invalid-layout` (drill 5), `tech-vanish-shots` (drill 6), `drill-time-limit` (drill 11) choose their row with down presses 20 frames apart. `e2e-smoke` and `e2e-title` never used Training or a drill, so they are unchanged. `boot-to-training` stays for the two new scenarios only.
- New `training-vanilla` (no rep log lines in 20 s of Training; shot shows Enemy: Stand, no score) and `training-pause-cpu` (Training's own TRAINING MENU, CP row changed Stand to Walk, resumed with the CPU walking).
- Drills are 10 reps long, so scenarios that waited 30 to 60 s for more reps in Training now end at the summary after 10 reps. Event entry puts rep 1 at about loop frame 144 instead of 61; `drill-end-no-input` got 300 more frames of wait for that.
- Run and confirmed: `training-vanilla`, `training-pause-cpu`, `event-restart-change-list` (drill 2, 2, 1, 1 starts, CSS after the chord), `drill-heights`, `drill-heights-range`, `drill-invalid-layout`, `drill-platform-layout` (stage=31, landing_y=54), `drill-time-limit` (drill_end at 6 reps), `tech-vanish-shots` (vanish line), `drill-end-no-input` (summary, restart), `tech-chase-first-rep`, `tech-option-pool-of-one`, `rep-outcome-no-input`, `rep-outcome-mash-attack`, `tech-option-player-right`. Not run: the other `tech-option-*`, `rep-*`, `rep-outcome-*` variants and `tech-chase-displace` (same prefix swap only).
- `py dev.py check` passes (matching, training, training-authoring) and the dev_tests pass. Spec status set to resolved and drill-select marked done in priorities.
