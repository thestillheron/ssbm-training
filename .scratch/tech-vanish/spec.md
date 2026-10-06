# Tech vanish: hide the opponent mid-tech

Status: ready-for-agent

## Problem Statement

To punish a tech on reaction, a player has to recognise which tech option it is within its first 7 frames. If they can watch the whole animation, they can get away with reacting late, and never build the early read. I want an option that forces the read: the opponent is visible for the start of its tech, then disappears until it becomes vulnerable, so the only way to be in the right place is to have read it early.

## Solution

A drill setting, **vanish**. When it's on, for tech in place, tech in and tech away the opponent is drawn normally for the first 7 frames of the tech. It's then hidden, model and shadow, until its first [vulnerable frame](../../docs/glossary.md#glossary_vulnerable_frames), when it reappears. The tech sound and the tech flash are kept. A [missed tech](../../docs/glossary.md#glossary_missed_tech) is never hidden. The visible frame count is a flat 7 for all curriculum drills.

## User Stories

1. As a player, I want the opponent to vanish after the first 7 frames of its tech when vanish is on, so that I'm forced to read the tech early.
2. As a player, I want it to reappear on its first vulnerable frame, so that I can see where it is when it's punishable.
3. As a player, I want its shadow to vanish with it, so that the shadow doesn't give away its movement.
4. As a player, I want to still hear the tech sound and see the tech flash, so that the real cues I'd have in a match are kept.
5. As a player, I want a missed tech never to vanish, so that I can still see it lying on the ground.
6. As a player, I want vanish off by default, so that I learn the techs fully visible first.
7. As a player, I want the vanish to be the same 7 frames in every drill, so that I build one consistent habit.
8. As a player, I want the vanish timing to be correct for every opponent character, so that it works with any matchup.
9. As a player, I want the opponent always to be visible again for the next rep, so that a vanish never carries over.
10. As the agent, I want the hide and show frames written to the rep log, so that I can check the timing exactly.
11. As the developer, I want the visible frame count defined as one named constant, so that changing it later is a one-line change.

## Implementation Decisions

- **Timing.** Count from the first frame of the tech state (the landing frame). Frames 1–7 are visible. Hide from frame 8 through the frame before the first vulnerable frame, as measured by rep-outcome. If the vulnerable frames start at or before frame 8 (unlikely), nothing is hidden. Applies only to tech in place, tech in and tech away.
- **Hiding** sets the fighter's invisible flag (which skips its model draw and fighter-attached extras) and its no-shadow flag. The game clears both on every motion state change, so the training module sets them again **every frame** while hidden, not once. Show clears both, and the reset at rep start always clears them.
- **Kept on purpose:** the tech sound and flash effect (a separate effect object, which the invisible flag doesn't hide anyway).
- **Name tag / magnifier.** If the off-screen magnifier or a name tag would give the position away, they follow the invisible flag in the game already. Confirm this when testing.
- **Setting.** Uses drills-as-data's vanish bool. The visible frame count is a named constant of 7, not a drill setting (per the session decision: flat 7 for now).
- **Rep log** gets `vanish_from` and `vanish_to` frames.

## Testing Decisions

- **Rep log seam:** for each tech option except missed tech, `vanish_from` = landing + 7 and `vanish_to` = `vuln_from` − 1. For a missed tech, no vanish lines. For vanish off, no vanish lines.
- **Shots:** one taken mid-vanish shows no opponent and no shadow, and one at the vulnerable frame shows it. Shot timing is approximate, so the log is the exact check and shots are a visual check.

## Out of Scope

- A variable visible frame count, or per-drill or player-adjustable counts (may come later).
- Hiding during the fall, or during the missed-tech getup.
- Muting sounds or hiding effects.
