# 02: Custom drill defaults to character drop heights

**What to build:** A custom drill with default settings drops the opponent from its [character drop height](../../../docs/glossary.md#glossary_character_drop_height), as the curriculum does. The drill's Character default replaces the custom base height of 40. Any change to the drop heights row in the settings menu makes the heights explicit and used exactly as set: changing its value, or switching between Fixed, List and Range. How the row labels a Character default is deferred, so for now the row shows and steps from a Character default exactly as it does today from a fixed 40.

**Blocked by:** 01

**Status:** resolved

- [ ] The custom drill starts on Character; its drops are at the opponent's table height
- [ ] Any change to the settings menu's drop heights row makes the heights explicit
- [ ] The row's display and stepping from the default are unchanged (Fixed 40, steps of 10)
- [ ] Scenario: custom drill defaults log `height_kind=character` and drops at the table height
- [ ] Scenario: a list set in the settings menu is used as set
- [ ] `dev.py check` passes

## Answer

Implemented on `character-drop-heights-int`; see the merge commit for this ticket.
