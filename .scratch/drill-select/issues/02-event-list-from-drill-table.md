# 02: Event list built from the drill table

**What to build:** The [event list](../../../docs/glossary.md#glossary_event_list) shows the drill table and nothing else. Each drill is its own event, numbered in curriculum order, with its name on the row and its description and pass mark in the description box. The vanilla events are hidden, and the list scrolls and pages only over our drills. Test drills appear in authoring builds only. Choosing any event plays that drill on its own stage, with the characters picked at the CSS. Picking an event writes a `menu` line to the rep log.

Text goes through the shared ASCII-to-SIS helper (rep-feedback ticket 01, if it has landed; otherwise extend the training code's existing conversion). The UnclePunch findings, "Menus" ("Event selection: the retail Event Match screen, patched"), list the event select screen functions to hook.

**Blocked by:** 01

**Status:** ready-for-agent

- [ ] The list shows exactly the drill table's drills in order, numbered, and no vanilla event
- [ ] Highlighting a drill shows its description and pass mark
- [ ] Each event starts its own drill on its own stage (rep log `drill_start` name and stage)
- [ ] Test drills are listed in authoring builds and absent from player builds
- [ ] The rep log has a `menu` line naming the chosen drill
- [ ] Scenario: Event Match → choose drill 2 → `drill_start` for drill 2, with a shot of the list
- [ ] Adding a drill to the table adds an event with no other change
- [ ] `dev.py check` passes
