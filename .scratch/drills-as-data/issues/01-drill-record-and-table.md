# 01: Drill record and table, rep loop reads the current drill

**What to build:** A drill is a data record in a const table compiled into the training build, with a name, a one-line description, its stage, and the fixed values the rep loop uses today (a single drop height and a tech option pool). The rep loop runs whichever drill is current, which for now is the first table entry. The tech-options pool and constants move into the drill. The rep log gets a `drill_start` line with the drill name and its settings so tests know which drill produced which reps. Rep behaviour is otherwise unchanged. This is the tracer bullet the other tickets build on.

**Blocked by:** None (can start immediately).

**Status:** resolved

- [x] A drill record type exists with name, description, stage, landing point, heights, player start offset, opponent facing, reset player, option pool, vanish, length and pass mark fields, and a const table holds at least one drill
- [x] The rep loop reads its height and option pool from the current drill instead of hardcoded values
- [x] The first table entry is the current drill
- [x] The rep log writes a `drill_start` line with the drill name and settings at drill start
- [x] Existing rep log scenarios still pass
- [x] `dev.py check` passes

## Answer

Added `src/training/drill.h` (the `Drill` record: name, description, stage, landing point, heights list/range, player start offset, opponent facing, reset player, option pool, vanish, length, pass mark) and `src/training/drill.c` (const table with one drill, "Tech in place"; `training_drill_current()` returns the first entry). `tech_chase.c` now reads the drop height and option pool from the current drill (the hardcoded `OPPONENT_DROP_HEIGHT` and `TECH_OPTION_POOL` default are gone) and logs `[rep] event=drill_start name=... stage=... height=... pool=... vanish=... reset=... length=kind/n pass=...` at Training entry. `dev.py --tech-pool` still works: the mask is now compiled into `drill.c` and overrides every drill's pool. Verified with `run --scenario tech-option-pool-of-one`: one drill_start line, then unchanged drop/tech lines (forced_ok=true). Only heights[0] and the first-entry drill are used so far; later tickets use the rest of the fields.
