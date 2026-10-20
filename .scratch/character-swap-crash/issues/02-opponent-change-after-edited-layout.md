# 02: Changing the opponent after editing the layout loads the match

**What to build:** The reported crash is fixed. Pick Marth against Falco on the [custom drill](../../../docs/glossary.md#glossary_custom_drill) on Final Destination, change Falco's drop height in [Edit layout](../../../docs/glossary.md#glossary_edit_layout), choose Change characters, swap the opponent to Jigglypuff and pick Final Destination: the match loads and the drill starts with the edited layout. Reproduce the crash first with a regression scenario and a twin that skips the layout edit. Find the cause from the Dolphin log (`diagnosing-bugs`) before choosing the fix. The spec lists the candidate causes: the stage select's loads, releasing the opponent's fighter files, and the edited session copy.

**Blocked by:** 01

**Status:** resolved

- [x] A regression scenario plays the reported steps and crashes before the fix (could not reproduce at this tip; see Comments)
- [x] A control scenario plays the same steps without the layout edit; record whether it crashed before the fix
- [x] The cause is written up under `## Comments` in this ticket
- [x] After the fix, the regression scenario's log shows a second `drill_start` on Final Destination with Jigglypuff as the opponent and the edited drop height, and a shot shows the match running
- [x] The control scenario passes too
- [x] Picking the same characters again still needs no reload; `event-change-characters` still passes
- [x] `custom-drill-edit-layout`, `custom-drill-to-stage-select`, `custom-drill-new-stage` and `custom-drill-defaults` still pass
- [x] `dev.py check` passes

## Comments

**Resolved (user, 2026-10-18):** the user tested the reported scenario by hand and it no longer crashes. The crash most likely came from a build without the previous fix's `loaded_cpu` check (`beb425296`). No source change was needed; the two scenarios stay as the regression and control. The agent diagnosis follows.

**Could not reproduce the crash at this tip (agent diagnosis, not a fix).** Left open: the first and third boxes (a regression scenario that crashes before the fix; the cause written up) are not ticked, and no source change was made. The other boxes are ticked because they hold at the unchanged tip.

Scenarios added (all committed): `custom-drill-change-opponent` (the report: Marth vs Falco on Final Destination, Edit layout drop height 40 to 50, B, Change characters, CP token Falco to Jigglypuff, Final Destination), `custom-drill-change-opponent-no-edit` (control), and the fragments `custom-drill-marth-falco-fd` and `custom-drill-opponent-to-jigglypuff`.

Results on the unchanged tip, fast-disc movie, Dolphin log read for `psInitDataBanks`/panics (none):
- Regression: three `drill_start` lines, `player=18 opponent=22 heights=40`, `heights=50`, then `player=18 opponent=15 heights=50`, all on stage 32 (Final Destination); shot `jigglypuff-fd` shows Marth vs Jigglypuff with the drill running. No crash.
- Control (no edit): `opponent=22` then `opponent=15`. No crash.
- Variants run once each as throwaway scenarios, all without a crash: a quick Start after the pick (`a`, 3 frames, Start); a second Change characters changing the player (Marth to Luigi) and picking Battlefield; same picks again, B out of the stage select and pick again, then Restart drill; and the regression with a normal (not fast) disc, with long waits and also with Start 3 frames after the drop and the stage pick soon after the stage select appeared. The slow-disc runs needed a different hand path (the CPU started on Ice Climbers, not Pichu) and also showed `opponent=15`, `heights=50`.
- Existing scenarios still pass: `event-change-characters` (Luigi, then the same picks again: `player=8`, `17`, `17`), `custom-drill-edit-layout`, `custom-drill-to-stage-select`, `custom-drill-new-stage`, `custom-drill-defaults`.

Reading `preload` in `src/training/event.c` against `lbdvd.c`/`gmclassic.c` gave no case this ticket's steps hit that the reload misses: a changed opponent with the same player takes the same branch as a changed player (reset the cache, load the player and wait, release the old fighters, then add the opponent and stage), and the layout edit touches only the training layout module, with no loads.

Not yet ruled out: the report came from a build without the previous fix's `loaded_cpu` check (added in the same commit as the preload reload) or from a setup this harness does not model (a real memory card with drill history, real disc, dual core, another Dolphin version). Ask the reporter for the build and for the Dolphin log of the crash (`psInitDataBanks` or a different signature) before designing a fix; if it is the old build, close this ticket with the new scenarios as the regression. Note for runs: the Dolphin log is shared, so a second agent's `dev.py run` interleaves its `[rep]` lines and its Dolphin steals focus; run scenarios one at a time and filter on `player=`.

Character select hand moves measured (movie, fast disc, from the clamped top-left corner with `stick-up+stick-left:82`, 4-frame waits between holds, 60 frames to settle before A), for ticket 03:
- Marth (third row, sixth tile): `right:14`, `down:16`, `down:3`, `right:23`.
- Jigglypuff (third row, fourth tile): `right:14`, `down:16`, `down:3`, `right:7`.
- Luigi (first row, third tile): `right:14`, `down:6` (from `event-change-characters`).
- Falco (second row, first tile): `right:5`, `down:10`.
- The CP token on Pichu (third row, second tile, the usual default opponent): `right:9`, `down:17`; lift it with A, move, A drops it (the token follows the hand; no B needed). The player's token is already in the hand on entering the character select (A drops it) and B lifts it off a character later. The default opponent depends on the save: Pichu in the usual snapshot.
