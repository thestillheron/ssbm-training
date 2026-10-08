# 04: Mode entry, pause, restart and return to menu

**What to build:** The findings section on how UnclePunch enters its own events from the main menu (it replaces Event Match), and how it pauses and restarts an event and returns to its menus. Recommendations go to the 'game mode of its own' item and to drill-select's in-drill controls.

**Blocked by:** 01 (Reference folder and licence)

**Status:** resolved

- [x] The section cites UnclePunch paths in `ref/` for each claim
- [x] Each finding is mapped to our decomp's named functions where one exists
- [x] The section ends with a recommendation that names its downstream spec
- [x] The section says which ideas carry over directly and which need translating from UnclePunch's injection toolchain

## Answer

UnclePunch enters through retail Event Match (`GM_EVENT`) by patching its scene states; pause uses the game's own pause flag, sound and HUD calls with the retail pause UI disabled; restart is retail retry or an A+B chord; Exit ends the match back to the event list. Recommendations: take over `GM_EVENT` for Game mode of its own, and for drill-select follow UnclePunch's pause conventions plus an explicit restart item. See [Mode entry, pause, restart and return to menu](../findings.md#mode-entry-pause-restart-and-return-to-menu).
