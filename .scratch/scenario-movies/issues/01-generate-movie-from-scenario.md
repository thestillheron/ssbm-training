# 01: Generate a movie from a scenario

**What to build:** `dev.py scenario <name> --movie <path>` expands a scenario (includes resolved) and writes a Dolphin movie that plays it frame-exactly from power-on. Each scenario frame is one emulated frame. Melee polls the pad twice per emulated frame, so each scenario frame becomes two identical controller-1 input entries (one named constant, `ENTRIES_PER_FRAME`): a hold of N frames writes 2N entries with those controls pressed, `wait:N` writes 2N neutral entries, and shots write nothing. The command prints the path and the movie length in frames. It needs no Dolphin, no pad bindings and no focus, and it works on any platform, like `--dry-run`. The generator is a new pure-logic module beside the scenario and pad modules, fed by the scenario module's frame-indexed steps rather than the millisecond key-event plan. See `../spec.md` (Implementation Decisions: frame model, control mapping, movie header).

Header values come from a reference movie written by the developer's own Dolphin. The untracked hand-recorded `boot-to-training.dtm` in the scenarios folder is a usable reference for field layout and settings. It was recorded from a savestate, so the generated header must clear that flag and must not clear the save either: the existing memory card holds the everything-unlocked save the scenarios depend on.

**Blocked by:** None (can start immediately)

**Status:** resolved

- [x] `scenario <name> --movie <path>` writes a movie and prints its path and frame count; nothing is sent to Dolphin
- [x] Header: DTM magic, game ID `GALE01`, one standard GameCube controller on port 1, power-on (no savestate), existing memory card kept, and frame/input counts consistent with the entries
- [x] Entry count equals total scenario frames × entries per frame (2), including frames from `include`d scenarios
- [x] Buttons `a b x y z start` set their bits; `l`/`r` set the digital bit and full analog trigger value; stick and C-stick directions give full deflection around neutral 128; D-pad directions set their bits
- [x] Combined controls merge into one pad state (diagonals included); opposing directions in one step are an error naming the step
- [x] `wait` entries and the frames outside holds are neutral (buttons up, sticks centred, triggers 0); holds land on exactly the right frame indices
- [x] Malformed steps, unknown controls, missing includes and include cycles fail with the existing dry-run errors and write no file
- [x] Generating works with no pad config / no keyboard bindings available
- [x] Routine dev tests (sandbox + fixture scenarios, as the scenario dry-run tests do) cover the above by reading the written file back through the CLI; `dev.py check` passes
