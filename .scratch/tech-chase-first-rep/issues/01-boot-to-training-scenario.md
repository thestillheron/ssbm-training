# Boot-to-training scenario

Status: resolved
Blocked by: —
Spec: ../spec.md (stories 15, 17)

## What to build

A committed, reusable `boot-to-training` scenario for `dev.py run --scenario`: boot → main menu → 1P → Training → pick characters with fixed inputs → Final Destination, ending in Training Mode with both fighters on stage. Later scenarios include it. Prior art: `boot-to-character-select`.

## Acceptance criteria

- [ ] `dev.py run --scenario boot-to-training` reaches Training Mode on Final Destination unattended.
- [ ] Shots at character select and in-stage are readable and confirm the state reached.
- [ ] Scenario is includable from another scenario.
- [ ] Focus permission asked before running (CLAUDE.md).
