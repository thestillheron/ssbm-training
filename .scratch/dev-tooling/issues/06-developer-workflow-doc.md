# 06: Developer workflow doc

Spec: `../spec.md`

**What to build:** Someone who has never seen the repo, and is new to decomp, can follow one doc from cloning to the "- T" title screen. The doc covers:
- Prerequisites per OS.
- Supplying a disc image: why it isn't in the repo, and which version is needed.
- `setup` / `build` / `run` / `check`.
- **Matching build** vs **training build**, linking to the glossary.
- Where training code and guarded hooks go.
- "Run `check` before merging to `master`".
- Troubleshooting: Wine quarantine, wrong disc SHA-1, Dolphin not found.

`CLAUDE.md` gets a short pointer to the doc. Upstream's README is left untouched.

**Blocked by:** 03, 04, 05

**Status:** ready-for-agent

- [ ] Following the doc on the Mac from a fresh clone reaches the "- T" title screen, and `check` passes.
- [ ] `CLAUDE.md` points to the doc.
- [ ] Upstream's README is unchanged.
