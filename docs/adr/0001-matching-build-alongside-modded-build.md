# Keep the matching build alongside the modded build

This fork adds training features to Melee, but we keep the upstream matching build (byte-identical to the original `main.dol`) working on every commit, and compile training features only in a separate modded build. The matching build is our regression check: it proves tooling changes and upstream merges haven't broken anything independently of our features. We also keep pulling from upstream `doldecomp/melee`, so training code lives in its own source directory and touches upstream files only through small, deliberate hooks.

## Considered Options

- **Drop matching and always build non-matching.** Simpler, but we'd lose the only cheap, exact signal that the untouched game is still intact, and upstream merges would get harder to check.
