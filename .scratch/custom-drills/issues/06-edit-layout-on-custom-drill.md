# 06: Edit layout on the custom drill

**What to build:** Edit layout in the pause menu works on the custom drill's working copy, and the edits are kept in the session's custom drill so restarts and re-entering Custom drill keep them. Print layout stays authoring-only. This also satisfies `.scratch/layout-editor/issues/08-custom-drills-edit-layout.md`; mark that ticket done when this lands.

**Blocked by:** 01

**Status:** resolved

- [x] Edit layout is reachable from a custom drill's pause menu and edits the custom drill
- [x] Edits persist across restart and re-entering Custom drill in the same session
- [x] Print layout and developer text are absent in player builds
- [x] Scenario: on a stage with platforms, move the landing point onto a platform; the log shows drops onto that surface
