# 05: Drill summary menu: Restart, Next drill, Event list

**What to build:** When a drill ends, the [drill summary](../../../docs/glossary.md#glossary_drill_summary) shows a menu built with the menu component:

- **Restart** restarts in place, as in the pause menu.
- **Next drill** plays the next drill in the table as the next event. It keeps both fighters and reloads the match scene, so the stage may change. It is disabled after the last drill.
- **Event list** works as in the pause menu.

Update the glossary's drill summary entry, which still says "Press A to restart the drill."

**Blocked by:** 04

**Status:** ready-for-agent

- [ ] The summary shows the three items alongside the existing score, pass mark and per-option results (shot)
- [ ] Restart starts the same drill from rep 1
- [ ] Next drill starts the next drill with the same fighters, on its own stage when that differs (rep log `drill_start` name and stage)
- [ ] Next drill is disabled (greyed and skipped) on the last drill
- [ ] Event list reopens the list on the drill just played
- [ ] Each choice writes a `menu` line
- [ ] The glossary's drill summary entry matches the new behaviour
- [ ] `dev.py check` passes
