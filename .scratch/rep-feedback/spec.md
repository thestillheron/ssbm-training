# Rep feedback: what happened, and how early or late

Status: ready-for-agent

## Problem Statement

A score tells me whether I succeeded, not why I failed. Was it a tech in or a tech away? Was I too early (attacking into intangibility) or too late (the opponent was already actionable)? Without that, I can't correct anything, and the drill turns into guessing. The game already knows the exact frames, so it should tell me.

## Solution

Between [reps](../../docs/glossary.md#glossary_rep) a short result shows:
- the [tech option](../../docs/glossary.md#glossary_tech_option) the opponent did
- the outcome
- the timing, for example:
  - "hit on vulnerable frame 2 of 6"
  - "whiffed during intangibility"
  - "hit 4f after actionable"
  - "no attack"

The end-of-drill summary adds a per-option breakdown. Presentation follows the UnclePunch research.

## User Stories

1. As a player, I want to see which tech option the opponent did after each rep, so that I can check my read.
2. As a player, I want to see whether the rep was a success, failure or void, so that every result is clear.
3. As a player, on a success, I want to see which vulnerable frame I hit on, out of how many, so that I know how much margin I had.
4. As a player, on a failure where I attacked while the opponent was intangible, I want to see "whiffed during intangibility" and by how many frames I was early, so that I know to wait.
5. As a player, on a failure where my hit landed after the opponent was actionable, I want to see how many frames late it was, so that I know to be faster.
6. As a player, on a failure where I didn't attack at all, I want to see that, so that I know I hesitated or misread.
7. As a player, on a void rep, I want to see "early: hit before landing", so that I learn to wait for the landing.
8. As a player, I want the result shown briefly and clearly, without blocking the next rep for longer than the usual pause, so that the drill keeps its pace.
9. As a player, I want the result placed where it doesn't hide the fighters, so that I can watch the end of the rep.
10. As a player, I want the end-of-drill summary to show my success rate per tech option, and how often I was early or late, so that I know what to practise.
11. As a player, I want the feedback to look like the menus and text in the rest of the mode, so that it feels like one product.
12. As the agent, I want the same timing classification written to the rep log, so that the on-screen text can be checked against it.

## Implementation Decisions

- **Timing classification**, computed from rep-outcome's frames:
  - **Success:** hit frame relative to the vulnerable frames (k of n).
  - **Early:** the player had an attack or grab out (a hitbox or grab box active) during the intangible part of the window and no hit connected. Report how many frames before the first vulnerable frame. If detecting "attack out" proves hard, fall back to "attacked during intangibility" based on the player being in an attack or grab state.
  - **Late:** keep watching after the rep's failure through the inter-rep pause. If a hit or grab by the player connects there, report how many frames after actionable. The outcome stays a failure.
  - **None:** no attack during the window.
  - **Void:** hit before landing.
- **Presentation:** text drawn with the game's text library, styled, placed and timed according to the UnclePunch research. Shown during the inter-rep pause and cleared at reset.
- **Summary:** extends drills-as-data's summary with per-option success rate and early/late counts.
- **Rep log** gets `timing` (success_k/n, early_Nf, late_Nf, none, void) on the outcome line.

## Testing Decisions

- **Rep log seam:**
  - A no-input scenario gives `timing=none` on every rep.
  - A scenario that attacks immediately after landing on a tech in place gives `early` (or a success, if it lands in the vulnerable frames).
  - A scenario that attacks very late gives `late` or `none`.
  - These are loose checks, since piloting timing is approximate. The exact check is that the on-screen text matches the logged classification.
- **Shots** of each result type and of the summary.

## Out of Scope

- Sound cues for success or failure.
- Rep history graphs, or saving results.
