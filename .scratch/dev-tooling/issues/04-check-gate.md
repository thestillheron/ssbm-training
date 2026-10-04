# 04: `check` gate

Spec: `../spec.md`

**What to build:** A developer runs `dev.py check` before merging to `master`, and gets a clear pass/fail result for each build:
- The **matching build** must pass the build's existing SHA-1 verification. Don't reimplement it.
- The **training build** must compile and link.

`check` prints a per-build summary and exits non-zero on any failure. It's the fork's substitute for CI.

**Blocked by:** 02

**Status:** ready-for-agent

- [ ] `check` passes on a clean tree.
- [ ] A test injects a training hook into upstream code without the compile-time guard, so it leaks into the matching build, and asserts that `check` fails with the matching build reported as failing. The test brings its own hook; it doesn't depend on ticket 03's marker.
- [ ] A test breaks the training build, for example with a compile error in the training source area, and asserts that `check` fails with the training build reported as failing.
- [ ] The tests restore the tree afterwards.
