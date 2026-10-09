# 03: Drop heights: select, adjust, add, remove

**What to build:** The d-pad selects a drop height (left/right) and raises or lowers it (up/down). X adds a copy of the selected height and Y removes it. Range heights are edited as two selectable heights (min and max). The opponent marker follows the selected height.

**Blocked by:** 01

**Status:** resolved

- [x] D-pad left/right selects, up/down adjusts the selected height, and the opponent marker follows
- [x] X adds a copy of the selected height; Y removes it; the last remaining height cannot be removed
- [x] Range heights are editable as separate min and max entries
- [x] Scenario: add a height, leave; rep log drop lines reflect the new set of heights

## Answer

`src/training/layout_editor.c`: `edit_heights` (d-pad left/right selects, held up/down adjusts 1 unit/frame within 0..200, a range's min stays at or below its max) and `add_remove_height` (X inserts a copy after the selected height and selects it, up to `DRILL_MAX_HEIGHTS`; Y removes it, never the last; both ignored for ranges, which are edited as heights[0] min and heights[1] max). Each change re-places the markers, so the opponent hangs at the selected height. Scenarios `layout-edit-heights` (drill 1: `drill_start` heights=40, then 40,70 with drops at both, then 50) and `layout-edit-range` (drill 3: `height_kind=range heights=20,110`, drops within it), documented in `docs/developer-workflow.md`.
