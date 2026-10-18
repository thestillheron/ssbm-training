# 03: Layout editor and Print layout on Character drills

**What to build:** The layout editor shows the opponent marker at the height it will actually drop from. When a drill is on Character, the marker hangs at the opponent's table height, and the values text shows that height. The first height change makes the session copy's heights explicit, starting from the table value and adjusted as usual: raising or lowering it with the d-pad, or adding one with X. Moving the landing point or the player start leaves the drill on Character, and so does closing the editor without a change. Revert on a curriculum drill puts the heights back to Character. On the custom drill it keeps them, as it does today. For a Character drill, Print layout prints the Character kind with no heights, in a form that pastes back into the drill table unchanged. For an explicit drill it prints the heights as now.

**Blocked by:** 01

**Status:** resolved

- [ ] On a Character drill the marker and values text show the opponent's table height
- [ ] The first height change (up/down, X add) makes the heights explicit, starting from the table value
- [ ] Moving the landing point or player start, or closing without a change, keeps Character
- [ ] Revert on a curriculum drill returns the heights to Character
- [ ] Print layout prints the Character kind for a Character drill; pasted back, the drill is unchanged
- [ ] Scenario: a raised height on a curriculum drill is used for the following drops
- [ ] Scenario: moving only the landing point leaves `height_kind=character`
- [ ] `dev.py check` passes

## Answer

Implemented on `character-drop-heights-int`; see the merge commit for this ticket.
