# 05: Summary: success rate and early/late counts per option

**What to build:** The drill summary, shown when a drill ends, gets a per-option breakdown so the player knows what to practise. Each tech option that came up shows its success rate, and how many of its reps were early and how many were late (counts from tickets 03 and 04). Options that never came up are left out. Keep it on screen with the existing summary lines (name, score, pass mark, pass or fail), using the shared text helper and the same style.

**Blocked by:** 03 (early), 04 (late).

**Status:** resolved

- [x] The summary shows, for each tech option that came up, its success rate and its early and late counts
- [x] Options that never came up are not listed
- [x] The counts agree with the rep log's `timing` fields for the drill (checked against the log of a full drill run)
- [x] A shot of the summary after a drill with at least one success, early and late, or as close as piloting allows
- [x] `dev.py check` passes

## Comments

- The summary keeps its name, score, pass mark and PASS/FAIL lines, then draws a smaller table (header `Option  Success  Early  Late`) with a row per option that had a counted rep: successes/reps and the rate, early count, late count. Options with no counted rep (never came up, or only voids) are skipped (`add_option_table` in `src/training/score.c`). The summary moved left (x 200 to 125) so the four columns fit.
- Early and late are counted per rep from its final timing class at the end of its pause (`training_score_record_timing`, called from `RepState_Pause` for a rep scored as a failure), so a failure that turns late during the pause counts as late only.
- The rep log gets an `option_summary` line per option that came up after `drill_end` (`option=... counted= successes= early= late=`), documented in developer-workflow.md's Rep log section.
- New scenario `rep-summary-timing` pilots drill 0 to S E L N S E L S L N. Logged: 3 successes, early_6f and early_5f, three late (32f/33f); `option_summary option=in-place counted=10 successes=3 early=2 late=3` agrees with the reps' final `timing` fields (checked with a script), and the `summary` shot reads In place 3/10 30%, 2, 3. With `--tech-pool all` the same inputs gave four rows (in-place 0/2 early 1 late 1, in 1/2, away 0/2, miss 0/4 late 2), again matching the log.
- "Options that never came up are not listed" is met in code (rows are filtered on counted reps) but not shown live: drill 0's pool is in place only, and the all-option run happened to draw every option.
- `rep-result-shots` re-run: reps 4 to 11 are now `late` (late_3f growing about 7f a rep) instead of `none`; its comment and the developer-workflow entry are updated.
- Review: the summary rows, `option_summary` lines and early/late counts are now keyed by what the opponent did (`rep.tech`, the same as the result's line 1), not the option aimed for; `option=` can also read `wall` or `ceil`. `OptionScore` became `TechScore`, `training_score_option` became `training_score_tech`. `rep-summary-timing` unchanged (in-place 3/10, early 2, late 3).
