# 03: Step 2: the four pair drills

**What to build:** Four more [curriculum](../../../docs/glossary.md#glossary_curriculum) drills straight after the one-option drills, in this order: missed tech + tech in place, tech in + missed tech, tech away + tech in place, tech in + tech away. Each is a drill table entry on Final Destination with the default layout, a two-option pool, [vanish](../../../docs/glossary.md#glossary_vanish) on, **70 reps** and a **pass mark of 56**, and a description that says what the drill trains and what to look for (the early read between the two options). The pairs tech in place + tech in and tech away + missed tech are left out on purpose.

**Blocked by:** 01 (vanish until actionable, all four options; the missed-tech pairs need a missed tech to vanish), 02 (the table the entries go into, and the drill-start scenario).

**Status:** resolved

- [x] The event list has the four pair drills after the four one-option drills, in the order above, each with a description
- [x] Each has the right two-option pool, vanish on, 70 reps and pass mark 56
- [x] A live run of each shows only its two options in the rep log, and a `vanish_from`/`vanish_to` line for every rep, missed tech included
- [x] The drill-start scenario from ticket 02 covers them and passes
- [x] `dev.py check` passes (ask the user before `dev.py run`, and `dev.py stop` afterwards)

Done on `curriculum-v1`. The `drill-pairs` scenario (new) played each pair for about 12 reps: only the drill's two options in the `drop` lines (pools 9, 10, 5, 6), and a `vanish` line for every rep that reached its landing, missed tech included. `drill-start-all` ran with all 22 drills and passed `reps --check-drill-starts`. The four drills sit at rows 5 to 8, which pushed the test drills and the Custom drill down four rows again; the scenarios, their comments and the developer-workflow doc have the new numbers.
