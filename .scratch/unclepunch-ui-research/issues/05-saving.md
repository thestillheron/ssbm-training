# 05: Saving settings and progress

**What to build:** The findings section on how UnclePunch saves settings and progress to the memory card. The recommendation goes to the later save item.

**Blocked by:** 01 (Reference folder and licence)

**Status:** resolved

- [x] The section cites UnclePunch paths in `ref/` for each claim
- [x] Each finding is mapped to our decomp's named functions where one exists
- [x] The section ends with a recommendation that names its downstream spec
- [x] The section says which ideas carry over directly and which need translating from UnclePunch's injection toolchain

## Answer

No save system of its own: a few settings bytes in unused `GmSaveData` padding, event progress in Event Match's own save fields, the game's normal save points, and separate card files only for Lab recordings. Recommendation for Memory card saves: named fields in that padding without changing its size, our own card file for bigger data, and a live check that vanilla still loads the save. See [Saving settings and progress](../findings.md#saving-settings-and-progress).
