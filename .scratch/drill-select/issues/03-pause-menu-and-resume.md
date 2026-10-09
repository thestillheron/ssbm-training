# 03: Our pause menu replaces the retail one (menu component, Resume)

**What to build:** During a drill event, Start freezes the game and opens our text pause menu instead of the retail pause. The menu has Resume, which unfreezes and continues the drill exactly where it was. It also shows the punish window length for the current opponent: a value measured on earlier reps this session, or "—" until one rep has measured it. Retail pause never runs in our events.

Build the reusable menu component here:

- items with a label, description, enabled flag and confirm callback
- a cursor
- a 9-row scrolling window that skips and greys disabled rows
- d-pad/stick with rapid-held input to move, A to confirm, B to go back (never unpauses), Start to close
- sounds 2, 1 and 0 for move, confirm and back
- the highlight shown by text colour

The UnclePunch findings give the pausing conventions and the freeze calls ("Pause"; "Recommendation for drill-select's in-drill controls"). Set `rules.disable_pausing`, then use the game's own pause flag, sound and HUD calls. Any human port's Start opens the menu, and the pausing port drives it.

The pause menu, the summary (ticket 05), layout-editor and custom-drills all use this component, so keep its interface small.

**Blocked by:** 01

**Status:** ready-for-agent

- [ ] Start in a drill event freezes the match (fighters, CPU input, rep timers) and shows our menu; the retail pause UI does not appear (shot)
- [ ] Resume continues the rep with no change to the score, clock or rep state (rep log)
- [ ] B in the menu does not unpause; only Start or Resume does
- [ ] The opponent's punish window is shown, or "—" before it has been measured
- [ ] Menu selections write `menu` lines to the rep log
- [ ] Scenario: start a drill → Start → shot → Resume → the drill carries on
- [ ] `dev.py check` passes
