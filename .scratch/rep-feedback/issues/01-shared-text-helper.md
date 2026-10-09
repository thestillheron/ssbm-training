# 01: Shared text helper for training on-screen text

**What to build:** A prefactor so the per-rep feedback text is easy to write. The rep loop and the score each have their own converter from ASCII to the full-width Shift-JIS the game's SIS font expects. Merge them into one shared helper in the training code, and add the punctuation the feedback needs (`/`, `:`, `-`, and anything else the strings in tickets 02 to 05 use). Also retest whether `HSD_SisLib_803A70A0` (Text_SetText) can make a line longer than the text first drawn on it, as UnclePunch does (see `.scratch/unclepunch-ui-research/findings.md`, "Text library and decomp names"). If it can, drop the placeholder rule in the score text. If it can't, record that in a comment so later tickets keep using placeholders. Nothing the player sees changes.

**Blocked by:** None (can start immediately).

**Status:** resolved

- [x] One ASCII-to-SIS helper is used by every piece of training on-screen text, and no copies are left
- [x] The helper converts letters, digits, space, `/`, `:` and `-`
- [x] The result of the Text_SetText growth retest is recorded, and the placeholder rule is removed or kept to match it
- [x] The score, the drill summary, the invalid-layout notice and the drill chooser list look the same as before (shots from the existing scenarios)
- [x] `dev.py check` passes

## Comments

- The helper is `src/training/text.c` / `text.h`: `training_text_create`, `training_text_add` (ASCII line at x, y, scale), `training_text_set` (replace a line) and `training_text_to_sis`. It converts every printable ASCII character except the backquote by indexing the SIS font's own table `lbl_8040C8C0` (it lists the full-width form of ASCII 0x20 to 0x7E in order, backquote left out), so `/ : - % . , ! ? ( ) + ' =` etc. all work; anything else becomes a space. Lines are cut at 63 characters (the text library formats into 128 bytes). The score, summary, early notice, notices, chooser list and title marker all use it; no hand-written Shift-JIS is left.
- **Text_SetText retest: growth works.** The score lines now start as "0/0" and "0%" with no placeholder and are updated with `HSD_SisLib_803A70A0`. `rep-outcome-score` shows "0/5" and then "0/10" with the "0%" line below intact (the first of two lines grew, so the tail shift path ran). The placeholder rule is removed.
- Shots before and after (`e2e-title`, `rep-outcome-score`, `drill-chooser`, `drill-invalid-layout --drill 4`, `rep-outcome-early-shots`) match, with one intended difference: the chooser list and invalid-layout notice converted `:` to a space before, so "Test: three heights" now shows its colon, as the summary already did.
- `drill-end-no-input`'s `summary` shot lands after the restart (it showed 0/0 both before and after this change: shots are timed on the wall clock); the summary itself is seen in `rep-outcome-score`'s last shot.
- Review: `text.h` gained `training_text_set_colour`, `training_text_centre` and `training_text_set_visible`; the rep result, the summary and the notice use them instead of calling `HSD_SisLib_803A74F0` or setting `default_alignment`/`hidden` directly.
