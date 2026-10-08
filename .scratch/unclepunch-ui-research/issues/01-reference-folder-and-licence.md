# 01: Reference folder and licence

**What to build:** `docs/developer-workflow.md` gets one line saying `ref/` holds reference source such as the UnclePunch repo, is gitignored, and is never read by the build. The UnclePunch source in `ref/` is confirmed readable, and the findings document (`findings.md` in this feature folder) is created with a licence section: UnclePunch's licence and what it allows us to copy or adapt.

**Blocked by:** None (can start immediately).

**Status:** resolved

- [x] The developer workflow documents `ref/` in one line
- [x] `ref/` is confirmed gitignored and nothing in the build reads from it
- [x] The findings document exists with a licence section quoting the licence and stating what reuse it permits
- [x] The findings document has an empty section for each of the questions in tickets 02 to 06

## Answer

UnclePunch's repo has no licence, so default copyright applies: learn from it and reimplement against our decomp's names, never copy its code, assets or text. Only ideas carry over. `ref/` is gitignored and documented in the developer workflow. See [Licence](../findings.md#licence).
