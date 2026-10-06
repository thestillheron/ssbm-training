# Authoring tools flag and the rep log

Status: ready-for-agent

## Problem Statement

Two needs come up as soon as reps exist. First, some features are for me as curriculum designer, not for players: printing a drill's layout, diagnostic text. They'd confuse and distract a player, so they mustn't be in what players run. Today there's no way to compile something into the training build only on request. Second, checking rep behaviour from screenshots is slow and imprecise: a screenshot can't show which frame the opponent landed on or how long its punish window was, and input from piloting is only roughly timed. The agent and tests need a precise, text-based record of what happened in each rep.

## Solution

Add [authoring tools](../../docs/glossary.md#glossary_authoring_tools): an option on the training build (`--authoring` on `dev.py build` and `dev.py run`) that compiles authoring-only features in. `run` turns it on by default, since the developer is its main user. `build` defaults to off, so the default build is what a player gets.

The first authoring tool is the [rep log](../../docs/glossary.md#glossary_rep_log): the training build writes one line per rep event, and `dev.py` collects those lines from Dolphin's log and prints them after a scenario (and on demand), so a rep can be checked as text.

## User Stories

1. As the developer, I want `dev.py build --authoring` to produce a training build with authoring tools, so that I can design curriculum drills.
2. As the developer, I want plain `dev.py build` to leave authoring tools out, so that the default build is exactly what a player would get.
3. As the developer, I want `dev.py run` to include authoring tools by default, so that my usual loop has them without extra typing.
4. As the developer, I want `dev.py run --no-authoring` to launch the player version, so that I can check what players see.
5. As the developer, I want switching authoring on or off to rebuild only the training code, not the whole game, so that switching is fast.
6. As the developer, I want `dev.py check` to compile the training build both with and without authoring tools, so that neither variant breaks unnoticed.
7. As a player, I want no authoring tools, menu items or diagnostic text in the build I play, so that nothing confusing gets in the way.
8. As the agent, I want each rep to write log lines for its events (drop, landing, tech option, punish window frames, outcome), so that I can check a rep's behaviour precisely as text.
9. As the agent, I want the rep log lines in a stable, machine-readable format (one event per line, a fixed prefix, key=value fields), so that tests can parse them.
10. As the agent, I want `dev.py scenario` and `run --scenario` to print the rep log lines written while the scenario played, so that one command gives me both shots and events.
11. As the agent, I want a `dev.py` command that prints the rep log since the last `run`, so that I can check reps during manual piloting.
12. As the agent, I want a clear error when Dolphin isn't writing the game's log output, saying what to change, so that a missing log is never mistaken for "no reps happened".
13. As the developer, I want the rep log set up without permanently changing my Dolphin settings, if Dolphin allows per-launch overrides, so that my normal Dolphin use is unaffected.
14. As the developer, I want the rep log only in authoring builds, so that player builds do no logging work.
15. As the developer, I want the authoring option documented in the developer workflow, so that I and future agents know when it's on.
16. As the agent, I want a check-only dry-run path for the rep log collection that works on a fixture log file, so that it's testable without Dolphin.

## Implementation Decisions

- **Flag.** `dev.py build` and `run` accept `--authoring` / `--no-authoring`. Defaults: `build` off, `run` on. `configure.py --training` takes a matching option that defines an authoring macro **only for the training library's objects**, so switching doesn't recompile the game. `dev.py` remembers the configured variant in its existing state file and reconfigures only on a change, as it does when switching between the matching and training builds.
- **Guarding.** Authoring-only code sits inside the authoring macro within training files. Upstream hooks never depend on it.
- **`check`.** Builds matching, then training without authoring, then training with authoring, and reports each in the summary.
- **Rep log emission.** The training build writes rep log lines through the game's debug print (OSReport) with a fixed prefix (e.g. `[rep]`) and space-separated `key=value` fields, one event per line, including a rep number and the game frame. The events start with: rep start, drop (height), landing (frame), tech option, vulnerable from/to, actionable (frame), outcome. Later features add fields; existing fields aren't renamed.
- **Rep log collection.** `dev.py` reads Dolphin's log file from the Dolphin user folder, keeps only prefixed lines written since the scenario (or `run`) started, and prints them. Dolphin needs file logging and the OS report log type turned on; prefer per-launch config overrides on Dolphin's command line from `run`. If those don't apply to logger settings, say exactly which settings to turn on.
- **Verify first (unverified assumption).** That OSReport from a bare booted training `main.dol` reaches Dolphin's log. Dolphin may need the game's symbol map to recognise the print function. Check this first: `run` can place the build's map where Dolphin looks for it. If no route works, fall back to drawing the latest rep log lines as on-screen text and reading them from shots, and record that in this spec.
- **Command.** The on-demand print is a new `dev.py` subcommand (name to choose at implementation, e.g. `reps`), following the existing subcommand style.
- **Docs.** The developer workflow gets an "Authoring tools" section and the rep log format.

## Testing Decisions

- Tests drive the real `dev.py` command line and check exit codes, output and files, as `tools/dev_tests/` already does.
- CLI tests: flag parsing and defaults for `build` and `run`, the variant recorded in dev state, `check` reporting both training variants, and rep log collection from a fixture log (only prefixed lines, only since the start marker, the missing-log error).
- Live check (opt-in, takes focus): `run --scenario tech-chase-first-rep` prints at least two reps' drop/landing/actionable lines.
- A guard test that the training build without authoring contains no rep log strings, in the spirit of the existing unguarded-leak check.
- Prior art: `tools/dev_tests/test_check.py`, `support.py`, the e2e test.

## Out of Scope

- Print layout (layout-editor adds it as the second authoring tool).
- macOS log collection beyond what falls out naturally. Piloting is Windows-only anyway.
- A release or packaging step for players (a later item in priorities).

## Further Notes

- Fields for punish window and outcome are filled in by rep-outcome. Until then the log shows drop, landing and actionable.
