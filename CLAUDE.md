## Agent skills

### Issue tracker

Issues live as local markdown files under `.scratch/<feature>/` (the fork has GitHub Issues disabled, and `gh` defaults to upstream). See `docs/agents/issue-tracker.md`.

### Triage labels

Default five roles (`needs-triage`, `needs-info`, `ready-for-agent`, `ready-for-human`, `wontfix`), recorded as a `Status:` line in each issue file. See `docs/agents/triage-labels.md`.

### Domain docs

Single-context: `docs/glossary.md` plus `docs/adr/` at the repo root. See `docs/agents/domain.md`.

### Developer workflow

Build, run and check with `dev.py` (`setup`, `build`, `run`, `check`); run `check` before merging to `master`. Matching build vs training build, and where training code and `TRAINING_BUILD` hooks go: see `docs/developer-workflow.md`.
