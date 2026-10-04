# 02: Boot the training build in Dolphin with `run`

Spec: `../spec.md`

**What to build:** A developer runs `dev.py run`, and their normal Dolphin install boots Melee from the **training build**.

- `configure.py` gains a minimal `--training` option. It implies non-matching, adds a training library from its own source area (empty for now, apart from a placeholder), and defines the compile-time training switch for all units.
- `dev.py build` now defaults to the training build, in its own output directory under `build/`. `dev.py` remembers which build is currently configured and re-runs `configure.py` only when switching builds, or when `configure.py` has changed. Switching back and forth doesn't recompile untouched code.
- `run` builds the training build, then assembles a playable game folder in the training output directory. The folder mirrors the extracted disc using links: hard links on Windows, falling back to copying with a warning. Only `main.dol` is replaced. `run` then launches Dolphin to boot it directly, without blocking on Dolphin's exit. The extracted original files are never written to.
- Introduce the gitignored per-machine config file, holding at least the Dolphin path. Auto-detect Dolphin at its standard macOS and Windows locations, with the config file as the override.

See ADR-0001 and ADR-0002.

**Blocked by:** 01

**Status:** ready-for-agent

- [ ] `build` produces the training build's `main.dol` in its own output directory. `build --matching` still produces the byte-identical matching build in upstream's default location.
- [ ] Alternating `build` and `build --matching` doesn't recompile untouched objects. Each repeat build is near-instant.
- [ ] A test overrides the Dolphin path with a fake executable that records its arguments. It asserts that `run` launched it with the assembled game's `main.dol`, and that the assembled folder holds the training build's `main.dol`.
- [ ] A test asserts the extracted original files' SHA-1 is unchanged after `run`.
- [ ] On the Mac, `run` boots the training build to Melee's title screen in Dolphin. This is a manual check.
