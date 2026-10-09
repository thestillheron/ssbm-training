# Priorities

The order of work for the tech-chasing curriculum. Each item is a feature with its own spec at `.scratch/<slug>/spec.md`. Work top to bottom; an item can start once everything in its `Blocked by` is done. Glossary terms: `docs/glossary.md`. Decisions: ADR-0003 (start inside Training Mode, superseded by ADR-0005: drills are Event Match events), ADR-0004 (force tech options through CPU input).

| # | Slug | Status | Blocked by | Summary |
|---|---|---|---|---|
| 1 | [tech-chase-first-rep](tech-chase-first-rep/spec.md) | done | — | In Training Mode the opponent drops in tumble, techs in place by fed input, and reps loop; `boot-to-training` scenario. |
| 2 | [authoring-tools-flag](authoring-tools-flag/spec.md) | done | 1 | `--authoring` build option (on by default for `run`) and the rep log, so reps can be checked as text. |
| 3 | [rep-outcome](rep-outcome/spec.md) | done | 1, 2 | Punish window and vulnerable frames measured live; success, failure, void; on-screen score. |
| 4 | [tech-options](tech-options/spec.md) | done | 3 | Tech in place, tech in, tech away, missed tech (fixed-time getup), uniform random from a pool. |
| 5 | [drills-as-data](drills-as-data/spec.md) | done | 4 | Drill records in a C table: layout with snapping, heights, pool, vanish flag, length, pass mark; drill end and summary. |
| 6 | [tech-vanish](tech-vanish/spec.md) | done | 3, 5 | Hide model and shadow from frame 8 of a tech until its first vulnerable frame. |
| 7 | [unclepunch-ui-research](unclepunch-ui-research/spec.md) | done | — | `ref/` folder; how UnclePunch does menus, feedback, mode entry and saves. |
| 8 | [rep-feedback](rep-feedback/spec.md) | done | 3, 4, 7 | Per-rep option, outcome, frames early/late; per-option summary. |
| 9 | [drill-select](drill-select/spec.md) | done | 5, 7 | Each drill an Event Match event; pause menu (resume, restart, change characters, event list); summary menu; Training Mode back to vanilla (ADR-0005). |
| 10 | [layout-editor](layout-editor/spec.md) | done | 2, 5, 9 | Edit layout in game with the fighters as markers; Print layout in authoring builds only. |
| 11 | [curriculum-v1](curriculum-v1/spec.md) | ready-for-human | 5, 6, 10 | Design and play-test the first curriculum along the agreed skeleton. |
| 12 | [custom-drills](custom-drills/spec.md) | done | 9, 10 | Custom drill settings menu, any stage, layout editor for players; session only. |

Item 7 is done; its findings ([unclepunch-ui-research/findings.md](unclepunch-ui-research/findings.md)) feed 8, 9 and the Later items.

## Later

Not ordered and not specced yet. Each gets a spec when triaged.

- **Game mode of its own** (needs-triage): mostly covered by drill-select taking over Event Match (ADR-0005). What remains is a main-menu entry and label of our own, if ever wanted.
- **Memory card saves** (needs-triage): persist custom drills, and possibly curriculum progress and locking.
- **Patch-based release** (needs-triage): a way for players to apply the training build to their own disc image, with authoring tools off.
- **Missed-tech getup options** (needs-triage): getup attack, getup rolls, and staying down after a missed tech, as a "Part 2" curriculum.
- **Per-matchup overrides** (needs-triage): adjust layouts or drop options for character pairs where a punish is impossible (e.g. Bowser vs Sheik tech away), driven by curriculum-v1 play-testing.
