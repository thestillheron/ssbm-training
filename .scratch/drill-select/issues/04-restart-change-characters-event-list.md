# 04: Restart drill, Change characters and Back to event list

**What to build:** The pause menu gains its remaining items:

- **Restart drill** resets the drill in place to rep 1 with a clean score and clock, without reloading the scene.
- **Change characters** ends the match and returns to the character select screen for the same event.
- **Back to event list** ends the match and reopens the event list with the drill just played highlighted.

L+R+A+Start during a drill also goes to the character select screen for the same event, as in vanilla matches. Retail Event Match sends it to the menu, so it has to be routed to the CSS instead. The findings' "Restart" and "Return to menu" sections cover the retail routing: the `onExitVs` outcomes and the "reopen on the last event" behaviour.

**Blocked by:** 02, 03

**Status:** ready-for-agent

- [ ] Restart drill logs a fresh `drill_start`, and the next rep is rep 1 with score 0
- [ ] Change characters reaches the CSS, and confirming starts the same drill (rep log)
- [ ] L+R+A+Start reaches the CSS for the same event
- [ ] Back to event list reopens the list on the drill just played (shot)
- [ ] Each choice writes a `menu` line
- [ ] Scenario: drill 2 → pause → restart → Back to event list → drill 1 → pause → Change characters → CSS → start → L+R+A+Start → CSS, with the `drill_start` names checked in the rep log
- [ ] Event Match's scores and cleared bits are still never written
- [ ] `dev.py check` passes
