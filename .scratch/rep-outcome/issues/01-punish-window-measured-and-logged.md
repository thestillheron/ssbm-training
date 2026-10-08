# 01: Punish window measured and logged

**What to build:** Each rep reads the opponent's live state every frame (motion state, animation frame, whole-body intangibility/invincibility, hurtbox states) and measures the [punish window](../../../docs/glossary.md#glossary_punish_window) and its [vulnerable frames](../../../docs/glossary.md#glossary_vulnerable_frames). The window opens on the landing frame (leaving tumble into a tech or knockdown state) and closes on the first frame the opponent is actionable. The window closing with no punish ends the rep as a failure. The rep lifecycle gains an *outcome* step between *landed* and the pause. The rep log gains `window_open`, `vuln_from`, `vuln_to`, `actionable` and `outcome` fields. Nothing is tabulated per character.

**Blocked by:** None (can start immediately)

**Status:** resolved

- [x] A scenario with no player input logs only `outcome=failure` reps
- [x] `window_open`, `vuln_from`, `vuln_to` and `actionable` are logged and match across reps for tech in place with the same layout and character
- [x] The window is measured from live state, with no per-character tables
- [x] The rep lifecycle has an *outcome* step between *landed* and the pause, and reps still loop
- [x] `dev.py check` passes

## Comments

Implemented in `src/training/tech_chase.c`: new `Landed` (measures the window each frame) and `Outcome` (logs, then Pause) states; vulnerable = no whole-body invincibility/intangibility (`x1988`, `x198C`, `x221D_b6`) and at least one hurtbox `Enabled`. The `outcome` log line carries `outcome=failure window_open vuln_from vuln_to actionable`, counted in frames from the rep's drop so they match across reps (`-1` = never). `hit_frame`/`hit_kind` and success/void are tickets 02/03. Animation frame is not read: motion state plus the intangibility/hurtbox flags were enough.

Verified: training, authoring and matching builds compile and `dev.py check` passes; the authoring dol contains the new fields; the new acceptance scenario `rep-outcome-no-input` expands in a dry run.

Unverified (needs `dev.py run --scenario rep-outcome-no-input`, which takes window focus and launches Dolphin): the first two criteria and that reps still loop. Then `dev.py reps` should show only `outcome=failure` lines with identical window fields.

Live run done: `dev.py run --scenario rep-outcome-no-input` logged 14 reps, all `outcome=failure window_open=31 vuln_from=31 vuln_to=87 actionable=87`, and reps looped. All criteria ticked.
