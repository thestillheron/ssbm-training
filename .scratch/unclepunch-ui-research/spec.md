# Research: how UnclePunch's training pack does menus and feedback

Status: ready-for-agent (once the developer has placed the UnclePunch source in `ref/`)

## Problem Statement

The drill menus and per-rep feedback should feel as clear as the UnclePunch training pack, which players already know. UnclePunch is open source and has solved the same problems: menus drawn on top of Melee, readable on-screen feedback, entering its own mode from the main menu, and saving settings. Designing these from scratch risks a worse result and wasted effort. Before building rep-feedback and drill-select, I want to know how UnclePunch does it and what we can reuse or copy in spirit.

## Solution

A gitignored `ref/` folder in the repo holds reference source such as the UnclePunch repo, so agents can read it alongside the decomp. A research pass (using the `research` skill) answers the questions below, with paths into `ref/` and into our decomp. The findings are written to this feature's folder, to be used by rep-feedback, drill-select and the later "game mode of its own" item.

## User Stories

1. As the developer, I want a gitignored `ref/` folder documented in the developer workflow, so that I know where to put reference source and that it's never committed.
2. As the agent, I want to read UnclePunch's source from `ref/`, so that research can quote real code paths.
3. As the developer, I want to know how UnclePunch draws its menus (its text and menu framework, whether built on the game's text library or its own), so that drill-select picks an approach that works.
4. As the developer, I want to know how UnclePunch shows per-attempt feedback (e.g. frame advantage, "early/late" text, colours, placement, duration), so that rep-feedback is just as readable.
5. As the developer, I want to know how UnclePunch enters its own events from the main menu (it replaces Event Match), so that the later "game mode of its own" item has a proven route.
6. As the developer, I want to know how UnclePunch pauses and restarts an event and returns to its menus, so that drill-select's in-drill controls match player expectations.
7. As the developer, I want to know how UnclePunch saves settings and progress to the memory card, so that the later save item has a starting point.
8. As the developer, I want to know whether UnclePunch has tech-chase-like events and how it forces CPU techs and hides fighters, so that we can compare with ADR-0004 and tech-vanish.
9. As the developer, I want to know UnclePunch's licence and what that allows us to copy or adapt, so that reuse is legitimate.
10. As the developer, I want each finding mapped to our decomp's named functions where one exists, so that findings turn straight into work.

## Implementation Decisions

- **`ref/`** is added to `.gitignore` with one line in the developer workflow saying what it's for. Nothing in the build reads from it.
- **Output:** a findings document in this feature folder, with one section per question (stories 3–10), each with UnclePunch paths, our decomp equivalents, and a recommendation for the downstream spec.
- **Note the toolchain gap.** UnclePunch is built as a code/data injection on top of the retail game, while we compile into the game itself. Findings should say which ideas carry over directly and which need translating.

## Testing Decisions

- None beyond review: the deliverable is a document. Check that every recommendation names a downstream spec.

## Out of Scope

- Implementing anything from the findings.
- Copying UnclePunch code without checking its licence.
