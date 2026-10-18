# 01: Character drop heights in the curriculum

**What to build:** Every curriculum drill drops the opponent from its [character drop height](../../../docs/glossary.md#glossary_character_drop_height) instead of a fixed 40. Each opponent character gets a row in a new per-character table, keyed by fighter kind. The starting values are in the spec, with a comment giving the derivation from max fall speed and saying the values are tuned by hand, and any character without a row falls back to 40. A new Character height kind sits alongside List and Range, and each rep on a Character drill picks its height from the opponent's table row. Every curriculum drill moves to Character, along with each authoring test drill that uses a single 40. The three heights, height range and no heights test drills keep their explicit heights. A drill on Character counts as unedited, so history doesn't mark it edited.

**Blocked by:** None (can start immediately)

**Status:** resolved

- [ ] Table with one row per playable character (Zelda and Sheik separate, Ice Climbers as Popo), with the spec's starting values and a derivation comment; fallback 40 for any other fighter kind
- [ ] Character height kind; a rep on a Character drill drops from the opponent's table height
- [ ] Curriculum drills and single-40 test drills on Character; explicit test drills unchanged
- [ ] Two Character drills have the same layout; a curriculum drill on Character is not "layout edited"
- [ ] Rep log: `drill_start` shows `height_kind=character`; each `drop` line adds the opponent's fighter kind and keeps the height actually used
- [ ] Rep-log parser and its test handle the new kind and field
- [ ] Static check, modelled on the drill ID check, fails if the table lacks a playable character (values not checked)
- [ ] Scenarios that expected `height=40` from the default CPU expect its table height instead; a curriculum drill scenario shows drops at the table height
- [ ] `dev.py check` passes

## Answer

Implemented on `character-drop-heights-int`; see the merge commit for this ticket.
