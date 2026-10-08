# 06: Tech-chase events and vanish

**What to build:** The findings section on whether UnclePunch has tech-chase-like events and how it forces CPU techs and hides fighters, compared with ADR-0004 and tech-vanish. The recommendation goes to tech-options or tech-vanish follow-ups, whichever fits.

**Blocked by:** 01 (Reference folder and licence)

**Status:** resolved

- [x] The section cites UnclePunch paths in `ref/` for each claim
- [x] Each finding is mapped to our decomp's named functions where one exists
- [x] The section ends with a recommendation that names its downstream spec
- [x] The section says which ideas carry over directly and which need translating from UnclePunch's injection toolchain

## Answer

No tech-chase drill and no fighter hiding. The Lab forces CPU techs by writing the tech-input timers every frame and feeds getups as CPU input. Recommendation: keep ADR-0004's fed input for tech-options (timer writing as a recorded fallback), use the Lab as prior art for Missed-tech getup options, and no change to tech-vanish. See [Tech-chase events and vanish](../findings.md#tech-chase-events-and-vanish).
