# 05: Scenario prints the rep log

**What to build:** `dev.py scenario` and `dev.py run --scenario` print the rep log lines written while the scenario played, alongside the shot paths, so one command gives the agent both shots and events. Collection reuses the `reps` command's reader, scoped to the scenario's start.

**Blocked by:** 03: `reps` command; 04: Tech-chase rep emits the rep log

**Status:** resolved

**Note:** the live check takes Dolphin window focus, so ask the user first.

- [ ] A scenario run prints only the rep lines written since that scenario started
- [ ] Fixture-based CLI tests cover the scenario output without Dolphin
- [ ] Opt-in live check: `run --scenario tech-chase-first-rep` prints at least two reps' drop, landing and actionable lines
- [ ] The developer workflow's scenario section mentions the rep log output

## Answer

Merged into authoring-tools-integration (final merge 84072fc42).
