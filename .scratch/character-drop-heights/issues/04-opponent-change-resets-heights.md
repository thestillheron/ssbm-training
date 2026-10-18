# 04: Opponent change resets drop heights to Character

**What to build:** When the opponent's character changes, the drop height goes back to that opponent's [character drop height](../../../docs/glossary.md#glossary_character_drop_height). The training code remembers the opponent's fighter kind from the last drill start. When a drill starts with a different one, the current drill's explicit heights go back to Character, whether they were set in the layout editor or in the custom drill's settings menu. For a curriculum drill that is the session copy, and for the custom drill it is the custom drill's record. Re-picking the same opponent keeps the edits, and so does changing only the player's character. This ticket also covers the layout editor on the custom drill, which can only be on Character once 02 has landed.

**Blocked by:** 02, 03

**Status:** resolved

- [ ] A drill started with a different opponent kind resets explicit heights to Character (curriculum session copy and custom drill)
- [ ] Same opponent re-picked, or only the player's character changed: explicit heights kept
- [ ] Layout editor on a custom drill on Character behaves as in 03 (marker at the table height, first height change makes heights explicit)
- [ ] Scenario: a height edited in the layout editor, then Change characters to a different opponent, drops at the new table height
- [ ] Scenario: a list set in the custom drill's settings menu resets after the opponent changes
- [ ] Scenario: Change characters with the same opponent keeps the edited height
- [ ] `dev.py check` passes

## Answer

Implemented on `character-drop-heights-int`; see the merge commit for this ticket.
