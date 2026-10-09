# 06: Print layout (authoring builds only)

**What to build:** In builds with authoring tools, a Print layout entry writes the edited layout to the rep log as a line with a layout prefix whose payload is the drill table's layout fields as C initializer text, ready to paste into the curriculum. Players never see it.

**Blocked by:** 02, 03

**Status:** resolved

- [x] Scenario in an authoring build: Print layout writes a layout line whose values match the edited layout
- [x] Non-authoring build has no Print layout entry (shot)
- [x] Guard test: no layout strings in a non-authoring build, as in authoring-tools-flag

## Answer

Print layout is the pause menu's last item (under Edit layout), compiled only with `AUTHORING_BUILD` (`print_layout` in `src/training/pause.c`); choosing it keeps the menu open. It writes one rep log line with the current drill's session layout as C initializer text, the fields `landing_x` to `start_dy` in struct Drill order, floats rounded to tenths with an `F` suffix:

```
[rep] layout: <landing_x>F, <landing_y>F, DrillHeight_List|DrillHeight_Range, <height_count>, { <h1>F[, <h2>F ...] }, <start_dx>F, <start_dy>F,
[rep] layout: 60.0F, 0.0F, DrillHeight_List, 2, { 40.0F, 70.0F }, 0.0F, 0.0F,
```

For ticket 07: a layout line is a line starting `[rep] layout: ` (not `key=value`, and distinct from `[rep] event=layout`); everything after that prefix is the pasteable text. `read_reps` already returns these lines. Format documented in `docs/developer-workflow.md` (Rep log, line format).

Scenario `tools/scenarios/layout-print` (drill 1): prints `[rep] layout: 0.0F, 0.0F, DrillHeight_List, 1, { 40.0F }, 0.0F, 0.0F,` then, after an edit, `[rep] layout: 60.0F, 0.0F, DrillHeight_List, 2, { 40.0F, 70.0F }, 0.0F, 0.0F,`, matching the edit's `drill_start`. With `--no-authoring` its shot `paused` shows the menu ending at Edit layout. Guard: `tools/dev_tests/test_authoring.py` checks `Print layout` and `[rep] layout: ` are in the authoring dol and absent from the player dol.
