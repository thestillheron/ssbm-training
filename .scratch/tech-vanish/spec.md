# Tech vanish: hide the opponent mid-tech

Status: resolved

Amended 2026-10-10 (curriculum-v1, issue 01): the opponent now reappears on its first actionable frame instead of its first vulnerable frame, and a missed tech vanishes too. Everything below is the amended behaviour.

## Problem Statement

To punish a tech on reaction, a player has to recognise which tech option it is within its first 5 frames. If they can watch the whole animation, they can get away with reacting late, and never build the early read. I want an option that forces the read: the opponent is visible for the start of its tech, then disappears until it can act, so the only way to be in the right place is to have read it early. The punish has to be committed to before the opponent becomes vulnerable, so showing it at the first vulnerable frame is no use; it reappears when the punish window closes, and the reappearance is the cue that the punish was late.

## Solution

A drill setting, **vanish**. When it's on, for every [tech option](../../docs/glossary.md#glossary_tech_option) (tech in place, tech in, tech away and [missed tech](../../docs/glossary.md#glossary_missed_tech)) the opponent is drawn normally for the first 5 frames of the tech. It's then hidden, model and shadow, until its first actionable frame, when it reappears (for a missed tech, the frame on which it can choose its getup option). The tech sound and the tech flash are kept. The visible frame count is a flat 5 for all curriculum drills.

## User Stories

1. As a player, I want the opponent to vanish after the first 5 frames of its tech when vanish is on, so that I'm forced to read the tech early.
2. As a player, I want it to reappear on its first actionable frame, so that its reappearance tells me my punish was late.
3. As a player, I want its shadow to vanish with it, so that the shadow doesn't give away its movement.
4. As a player, I want to still hear the tech sound and see the tech flash, so that the real cues I'd have in a match are kept.
5. As a player, I want a missed tech to vanish the same way as a tech, so that it can't be told apart from the techs by whether it disappears.
6. As a player, I want vanish off by default, so that I learn the techs fully visible first.
7. As a player, I want the vanish to be the same 5 frames in every drill, so that I build one consistent habit.
8. As a player, I want the vanish timing to be correct for every opponent character, so that it works with any matchup.
9. As a player, I want the opponent always to be visible again for the next rep, so that a vanish never carries over.
10. As the agent, I want the hide and show frames written to the rep log, so that I can check the timing exactly.
11. As the developer, I want the visible frame count defined as one named constant, so that changing it later is a one-line change.

## Implementation Decisions

- **Timing.** Count from the first frame of the tech state (the landing frame). Frames 1–5 are visible. Hide from frame 6 through the frame before the first actionable frame, as measured by rep-outcome (the punish window's last frame). If the opponent is actionable at or before frame 6 (unlikely), nothing is hidden. Applies to all four tech options.
- **Hiding** sets the fighter's invisible flag (which skips its model draw and fighter-attached extras) and its no-shadow flag. The game clears both on every motion state change, so the training module sets them again **every frame** while hidden, not once. Show clears both, and the reset at rep start always clears them. A hit or grab that ends the rep also shows the opponent (the window closes).
- **Kept on purpose:** the tech sound and flash effect (a separate effect object, which the invisible flag doesn't hide anyway).
- **Name tag / magnifier.** If the off-screen magnifier or a name tag would give the position away, they follow the invisible flag in the game already. Confirm this when testing.
- **Setting.** Uses drills-as-data's vanish bool. The visible frame count is a named constant of 5, not a drill setting (per the session decision: flat 5 for now).
- **Rep log** gets `vanish_from` and `vanish_to` frames.

## Testing Decisions

- **Rep log seam:** for every tech option, `vanish_from` = landing + 5 and `vanish_to` = `actionable` − 1. For vanish off, no vanish lines.
- **Shots:** one taken mid-vanish shows no opponent and no shadow, and one at the actionable frame shows it. Shot timing is approximate, so the log is the exact check and shots are a visual check.

## Out of Scope

- A variable visible frame count, or per-drill or player-adjustable counts (may come later).
- Hiding during the fall.
- Hiding after the opponent is actionable, including the getup that follows a missed tech.
- Muting sounds or hiding effects.
