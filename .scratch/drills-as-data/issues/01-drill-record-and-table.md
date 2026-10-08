# 01: Drill record and table, rep loop reads the current drill

**What to build:** A drill is a data record in a const table compiled into the training build, with a name, a one-line description, its stage, and the fixed values the rep loop uses today (a single drop height and a tech option pool). The rep loop runs whichever drill is current, which for now is the first table entry. The tech-options pool and constants move into the drill. The rep log gets a `drill_start` line with the drill name and its settings so tests know which drill produced which reps. Rep behaviour is otherwise unchanged. This is the tracer bullet the other tickets build on.

**Blocked by:** None (can start immediately).

**Status:** ready-for-agent

- [ ] A drill record type exists with name, description, stage, landing point, heights, player start offset, opponent facing, reset player, option pool, vanish, length and pass mark fields, and a const table holds at least one drill
- [ ] The rep loop reads its height and option pool from the current drill instead of hardcoded values
- [ ] The first table entry is the current drill
- [ ] The rep log writes a `drill_start` line with the drill name and settings at drill start
- [ ] Existing rep log scenarios still pass
- [ ] `dev.py check` passes
