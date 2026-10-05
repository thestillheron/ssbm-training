# 07: Verify end to end on Windows

Spec: `../spec.md`

**What to build:** On the Windows machine, from a fresh clone, Jamie follows the developer workflow doc: `py dev.py setup`, then `run`, then `check`. Any failures are filed as new tickets in this directory.

**Blocked by:** 06

**Status:** resolved

- [ ] Melee boots in Dolphin from the training build, showing "- T".
- [ ] `check` passes on Windows.
- [ ] This works without WSL, msys2 or admin rights.

## Comments

Resolved on Windows. macOS verification (setup from RVZ, Wine/Rosetta, run, check) was deferred; track it as a later ticket.
