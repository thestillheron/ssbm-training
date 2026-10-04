# 07: Verify end to end on Windows

Spec: `../spec.md`

**What to build:** On the Windows machine, from a fresh clone, Jamie follows the developer workflow doc: `py dev.py setup`, then `run`, then `check`. Any failures are filed as new tickets in this directory.

**Blocked by:** 06

**Status:** ready-for-human

- [ ] Melee boots in Dolphin from the training build, showing "- T".
- [ ] `check` passes on Windows.
- [ ] This works without WSL, msys2 or admin rights.
