# 02: Step 1: the four one-option drills, and the drill-start check

**What to build:** The first four drills of the [curriculum](../../../docs/glossary.md#glossary_curriculum), in this order: tech in place, tech in, tech away, missed tech. Each is a [drill table](../../../docs/glossary.md#glossary_drill_table) entry on Final Destination with the default layout, one [tech option](../../../docs/glossary.md#glossary_tech_option), vanish off, **35 reps** and a **pass mark of 28**, and a description that says what the drill trains and what to look for. They replace the single curriculum drill the table has now (tech in place, 10 reps, pass mark 7), so the event list starts with these four. The authoring-only test drills stay, but they now sit after the curriculum drills, so every acceptance scenario that picks a test drill by its position in the event list must be updated to the new position (search `dev.py` and `docs/developer-workflow.md` for `--drill` and "drill N"). Update the developer-workflow doc where it names those positions.

Add a scenario that starts every drill once, in the curriculum and the test drills alike, and checks each `drill_start` line in the rep log for errors (for example an invalid layout reported where none is expected, and the test drills that are meant to be invalid reported as such). Later curriculum tickets add their drills to the table and the scenario picks them up.

**Blocked by:** None (can start immediately).

**Status:** resolved

- [x] The event list's first four events are tech in place, tech in, tech away and missed tech, each with a description
- [x] Each of the four has a single-option pool, vanish off, 35 reps and pass mark 28, and is not an invalid layout
- [x] The former single curriculum drill is gone, and no scenario or doc still refers to the old drill positions
- [x] A scenario starts every drill once and fails on an unexpected error in any `drill_start` line, while accepting the intentionally invalid test drills
- [x] `dev.py check` passes, and the new scenario passes in a live run (ask the user before `dev.py run`, and `dev.py stop` afterwards)

## Comments

Rep counts and pass marks come from the length maths in [the spec](../spec.md): ~200 frames per rep, 120 of them the fixed pause, so 2 minutes is about 35 reps and the pass mark is 80%.

Done on `curriculum-v1`. `drill-start-all` ran live (all 18 table drills started; `Test: invalid layout` and `Test: no heights` reported their intended problems) and `python dev.py reps --check-drill-starts` passed. The check is a new `reps` flag (`tools/reps.py` `check_drill_starts`, unit tests in `tools/dev_tests/test_reps.py`); the Custom drill is not in the table and is covered by the `custom-*` scenarios.

Three first-drill scenarios (`drill-end-no-input`, `rep-result-shots`, `rep-summary-timing`) assume 10 reps and pass mark 7, which the first drill no longer has (35 and 28), so they now choose a new authoring-only test drill, `Test: ten reps` (drill 5, the old first drill). That pushed every test drill and the Custom drill down four rows, so the scenarios that choose them by position have four more `stick-down`s and their comments and `docs/developer-workflow.md` give the new numbers. Ticket 03 shifted them by four again.
