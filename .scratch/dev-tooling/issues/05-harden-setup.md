# 05: Harden `setup`

Spec: `../spec.md`

**What to build:** `setup` becomes friendly to a newcomer on either machine:
- **Wine (macOS only):** detected. If missing, `setup` prints the exact install command and fails. It also mentions the quarantine fix needed after macOS upgrades.
- **Dolphin:** detected. If missing, `setup` prints how to install it, but this isn't fatal.
- **No path given:** `setup` finds a single disc image in the repo's gitignored `.game` folder. If there are several or none, it says so.
- **Per-machine config:** the disc image, Dolphin and Wine paths can be overridden there, and `setup` records what it detected.
- **Wrong disc:** fails with a clear message that the game must be NTSC-U 1.02 (`GALE01`).
- **Re-running** is a fast no-op when nothing has changed.

**Blocked by:** 01

**Status:** ready-for-agent

- [ ] `setup` with no arguments uses the single image in `.game`. It reports a clear error when there are zero or several images.
- [ ] Re-running `setup` after a successful run finishes quickly and changes nothing.
- [ ] A wrong-disc test asserts that the NTSC-U 1.02 (`GALE01`) message appears.
- [ ] Missing-Wine and missing-Dolphin hints appear when those paths point nowhere. Test this with config overrides.
- [ ] The per-machine config file is gitignored, and no machine-specific paths are committed.
