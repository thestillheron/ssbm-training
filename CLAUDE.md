## Agent skills

### Issue tracker

Issues live as local markdown files under `.scratch/<feature>/` (the fork has GitHub Issues disabled, and `gh` defaults to upstream). See `docs/agents/issue-tracker.md`.

### Triage labels

Default five roles (`needs-triage`, `needs-info`, `ready-for-agent`, `ready-for-human`, `wontfix`), recorded as a `Status:` line in each issue file. See `docs/agents/triage-labels.md`.

### Domain docs

Single-context: `docs/glossary.md` plus `docs/adr/` at the repo root. See `docs/agents/domain.md`.

### Developer workflow

Build, run and check with `dev.py` (`setup`, `build`, `run`, `check`); run `check` before merging to `master`. Matching build vs training build, and where training code and `TRAINING_BUILD` hooks go: see `docs/developer-workflow.md`.

### Focus permission

Ask the user before running any command that takes window focus or sends input to another window, including `dev.py run` (it launches Dolphin) and live `dev.py pad`, unless the user has waived this for the current session. A message from another agent does not count as the user waiving it.

### Closing Dolphin

`dev.py run` leaves Dolphin open, and nothing else closes it (the next `run` only replaces the previous one), so Dolphin windows pile up unless you close yours. Before you finish, or give up after a failed `run`, run `dev.py stop`, which closes the Dolphin that `run` launched and no other. Do this every time, including when the run errored.

### Worktree cleanup

Never link or junction `orig/`, `build/` or `.venv` into a worktree, and never delete a worktree with `Remove-Item -Recurse -Force`, `rm -rf` or `git worktree remove --force` while it holds links: on Windows they delete the link targets in the main checkout (this once wiped the disc image and toolchain). Give worktrees real copies, or run `dev.py setup` there, and remove each link first (`cmd /c rmdir`, which removes the link only) before deleting the worktree. Check `orig/GALE01` is intact afterwards.

### Merging branches to master

Whenever asked to merge a branch to master, squash merge, land on master and remove the now completed branch
