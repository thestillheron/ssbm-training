# 01: Authoring flag

**What to build:** `dev.py build` and `dev.py run` accept `--authoring` / `--no-authoring` (`build` defaults off, `run` defaults on). `configure.py --training` takes a matching option that defines an authoring macro only for the training library's objects, so switching variants doesn't recompile the game. `dev.py` records the configured variant in its state file and reconfigures only on a change. `check` builds matching, then training without authoring tools, then training with them, and reports each in its summary. A tiny authoring-only guarded marker in a training file proves the macro reaches training code and never the player build. The developer workflow gets an "Authoring tools" section.

**Blocked by:** None (can start immediately)

**Status:** resolved

- [ ] `build` defaults to no authoring tools; `build --authoring` includes them
- [ ] `run` defaults to authoring tools on; `run --no-authoring` launches the player variant
- [ ] Switching variants rebuilds only training objects, not the game
- [ ] The configured variant is recorded in dev state; reconfigure happens only on change
- [ ] `check` reports matching, training and training-with-authoring, and exits non-zero if any fails
- [ ] Authoring-only code is guarded by the authoring macro; upstream hooks never depend on it
- [ ] CLI tests cover flag parsing and defaults, the recorded variant, and the `check` summary
- [ ] The developer workflow documents the Authoring tools section

## Answer

Merged into authoring-tools-integration (final merge 84072fc42).
