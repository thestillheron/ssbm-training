# 02: Menus

**What to build:** The findings section on how UnclePunch draws its menus: its text and menu framework, whether built on the game's text library or its own. The recommendation goes to drill-select.

**Blocked by:** 01 (Reference folder and licence)

**Status:** resolved

- [x] The section cites UnclePunch paths in `ref/` for each claim
- [x] Each finding is mapped to our decomp's named functions where one exists
- [x] The section ends with a recommendation that names its downstream spec
- [x] The section says which ideas carry over directly and which need translating from UnclePunch's injection toolchain

## Answer

Event selection is the retail Event Match screen, patched; the in-event menu is UnclePunch's own data-driven C menu drawn with the game's text library (SisLib), with no font of its own. Recommendation for drill-select: a cut-down version of that menu in our own code inside Training Mode, text-only first. See [Menus](../findings.md#menus).
