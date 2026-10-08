# Rep outcome: success, failure, void, and a score

Status: resolved

## Problem Statement

Reps loop, but the game doesn't know whether I punished the tech. A training drill is pointless if I can't tell whether I succeeded, and the curriculum needs pass marks, which need a count of successes. "Success" also has to mean exactly what the skill is: hitting the opponent after it lands and before it can act. Hitting it out of the air, or after it's actionable, doesn't count.

## Solution

Every [rep](../../docs/glossary.md#glossary_rep) ends with an outcome:
- **Success:** the player's first hit or grab connects inside the [punish window](../../docs/glossary.md#glossary_punish_window), between the opponent landing and its first actionable frame.
- **Failure:** the opponent becomes actionable without having been punished.
- **Void:** the player hits it before it lands. The rep doesn't count, "early" is shown, and it re-drops.

Whiffs don't end the rep. The window, and the [vulnerable frames](../../docs/glossary.md#glossary_vulnerable_frames) inside it, are measured from the opponent's actual state every frame, so it's exact for every character. A running score is on screen, and every outcome goes to the rep log.

## User Stories

1. As a player, I want a rep to count as a success when my hit connects after the opponent lands and before it can act, so that I'm rewarded for exactly the skill being trained.
2. As a player, I want a grab inside the punish window to count as a success, so that grab punishes count as they do in real play.
3. As a player, I want the rep to end the moment I succeed, so that I'm not left juggling for nothing.
4. As a player, I want a rep to count as a failure when the opponent becomes actionable unpunished, so that being too late is clearly a miss.
5. As a player, I want a hit on the opponent while it's still tumbling to void the rep and show "early", so that I learn to wait for the landing instead of hitting it out of the air.
6. As a player, I want a void rep to re-drop without counting, so that my score reflects only real attempts.
7. As a player, I want whiffs, including attacks during the tech's intangibility, not to end the rep, so that I can still react within the window.
8. As a player, I want a hit after the opponent is actionable not to count, so that a late hit isn't scored as a punish.
9. As a player, I want the punish window to be exact for whichever opponent character I picked, so that I can trust the result.
10. As a player, I want the score (successes out of counted reps) always on screen, so that I can see how I'm doing.
11. As a player, I want the score to count only my hits, not anything else that might hit the opponent, so that the score is mine.
12. As a player, I want a missed-tech landing to count too: a hit while it lies on the ground or gets up counts until it's actionable, so that missed techs are punishable as in real play.
13. As the agent, I want each outcome written to the rep log with the frames of landing, first vulnerable frame, last vulnerable frame, actionable frame and the hit frame, so that I can check outcomes precisely.
14. As the agent, I want a void rep logged as void with the frame of the early hit, so that early hits are visible in tests.
15. As the developer, I want the window measured from the opponent's live state rather than per-character tables, so that it stays right for all characters without data entry.

## Implementation Decisions

- **Measured live, not tabulated.** Each frame the training module reads the opponent's motion state, current animation frame, its whole-body intangibility/invincibility state and its hurtbox states. The **punish window** opens on the landing frame (leaving tumble into a tech or knockdown state) and closes on the first frame the opponent is back in an actionable state. Its **vulnerable frames** are the window frames in which it's neither intangible nor invincible. The game sets these from per-character data at runtime, so no tables are needed.
- **Detecting a punish.** Poll each frame:
  - **Hit:** the opponent took damage this frame, and the recorded attacker is the player's slot.
  - **Grab:** the opponent entered a captured state, and its grabber is the player's fighter. The game doesn't credit grabs to the attacker slot, so use the grabber link.
  - Polling is preferred over damage and grab callbacks, which the game resets per state.
- **Outcome rules:**
  - A punish during the window → success. The rep ends, and the opponent is then left to the game's normal hitstun/grab behaviour with neutral fed input, until the inter-rep pause ends.
  - The window closing without a punish → failure.
  - A punish before landing → void: no count, show "early", re-drop after the pause.
  - A hit during the inter-rep pause is ignored.
- **Score.** Successes / counted reps, drawn as text with the game's text library, as the title marker does.
- **Rep log.** Adds `window_open`, `vuln_from`, `vuln_to`, `actionable`, `hit_frame`, `hit_kind` (hit/grab) and `outcome` (success/failure/void) fields.
- **Rep lifecycle** gains an *outcome* step between *landed* and the pause.

## Testing Decisions

- **Rep log seam.** The opponent's side is checked exactly: for a tech in place by a given character, `vuln_from`/`vuln_to`/`actionable` match across reps (same layout, same character). A scenario with no player input produces only failures. One that mashes an attack during the fall produces voids.
- **Player timing is checked loosely**, since piloting is only roughly timed: a scenario that throws out grab attempts produces at least one success or failure line with `hit_kind=grab` where the grab connected.
- Shots confirm the score text is drawn.
- Prior art: the tech-chase-first-rep scenario and the authoring-tools-flag rep log tests.

## Out of Scope

- Telling the player which option it was, or how early/late they were (rep-feedback).
- Rep counts, drill end and pass marks (drills-as-data).
- Tech options other than tech in place. The rules here must already hold for missed tech, and tech-options will exercise that.

## Further Notes

- Grabs set the victim's "last attacker" to none, which is why grabs are detected through the grabber link.
