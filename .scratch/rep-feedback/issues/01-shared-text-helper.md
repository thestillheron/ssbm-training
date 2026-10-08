# 01: Shared text helper for training on-screen text

**What to build:** A prefactor so the per-rep feedback text is easy to write. The rep loop and the score each have their own converter from ASCII to the full-width Shift-JIS the game's SIS font expects. Merge them into one shared helper in the training code, and add the punctuation the feedback needs (`/`, `:`, `-`, and anything else the strings in tickets 02 to 05 use). Also retest whether `HSD_SisLib_803A70A0` (Text_SetText) can make a line longer than the text first drawn on it, as UnclePunch does (see `.scratch/unclepunch-ui-research/findings.md`, "Text library and decomp names"). If it can, drop the placeholder rule in the score text. If it can't, record that in a comment so later tickets keep using placeholders. Nothing the player sees changes.

**Blocked by:** None (can start immediately).

**Status:** ready-for-agent

- [ ] One ASCII-to-SIS helper is used by every piece of training on-screen text, and no copies are left
- [ ] The helper converts letters, digits, space, `/`, `:` and `-`
- [ ] The result of the Text_SetText growth retest is recorded, and the placeholder rule is removed or kept to match it
- [ ] The score, the drill summary, the invalid-layout notice and the drill chooser list look the same as before (shots from the existing scenarios)
- [ ] `dev.py check` passes
