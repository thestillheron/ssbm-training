# 07: Synthesis and toolchain gap

**What to build:** A summary at the top of the findings document that states, across all sections, which ideas carry over directly and which need translating, given that UnclePunch is a code/data injection on the retail game while we compile into the game itself. Every recommendation is checked to name a downstream spec, and every finding to have a mapped decomp function where one exists. The row for this item in `.scratch/priorities.md` is marked done.

**Blocked by:** 02, 03, 04, 05, 06 (the findings sections).

**Status:** resolved

- [x] The summary lists direct carry-overs and translations needed, each pointing to its section
- [x] Every recommendation in the document names a downstream spec (rep-feedback, drill-select, game mode of its own, or save)
- [x] The priorities row for unclepunch-ui-research is marked done

## Answer

The Summary gives the Event Match verdict (right for entry, the event list and progress; wrong for in-event menus and feedback), the toolchain gap as one rule (address patches become one-line `TRAINING_BUILD` hooks against named functions), tables of what carries over and what needs translating with their downstream specs, and the open questions handed on. The priorities row is done. See [Summary](../findings.md#summary).
