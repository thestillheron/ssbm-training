# Spec: launch straight to the main menu

Status: ready-for-agent

## Problem Statement

When a player boots RTC Training, they sit through the intro FMV and the title screen, and must press Start twice, before they reach the [training build](../../docs/glossary.md#glossary_training_build)'s main menu (ADR-0007). The training build has no 1P/VS game to advertise, so both screens are dead time. On a fresh save it is worse: the build unlocks every character and stage on a new save (ADR-0006), and Melee then queues its own unlock notices (random stage select, sound test, all characters, all stages, a trophy) which the player must dismiss one by one, eight presses, before the menu is usable. They are notices about things the player never earned and can't act on.

## Solution

The game launches straight to the main menu. No intro FMV, no title screen, no press needed. On a fresh save, the unlock notices never appear: the player goes directly to the main menu, with everything already unlocked. Everything else about the boot is unchanged: the create-save prompt on a card with no save for this build still shows (the player can still decline), and the main menu is the same one with the cursor on RTC 101.

## User Stories

1. As a player, I want the game to open on the main menu, so that I can start training without waiting.
2. As a player, I want no intro FMV, so that I don't sit through a movie I can't use.
3. As a player, I want no title screen, so that I don't have to press Start to get past it.
4. As a first-time player with a fresh save, I want no unlock notices, so that I'm not made to dismiss messages about things I didn't earn.
5. As a first-time player, I still want everything unlocked, so that every character and stage is available to the drills.
6. As a first-time player, I want the main menu to be ready for input as soon as it appears, so that my first press isn't swallowed by a notice.
7. As a returning player, I want boot to behave the same as for a first-time player, so that there is one consistent launch.
8. As a player with no memory card, I want Melee's "Continue without saving or loading Game Data?" prompt kept, so that I can still choose how to play.
9. As a player with a card that holds no save for this build, I want the create-save prompt kept, so that I can decline saving as in vanilla.
10. As a player who declines the save, I want to land on the main menu afterwards, with nothing queued behind it.
11. As a player, I want the main menu's cursor on RTC 101 on launch, as it is today.
12. As a player, I want the unlock notices to stay gone on later launches too, so that nothing turns up the next time I boot.
13. As a player coming back from a drill, the event list or Options and Data, I want those paths unchanged, so that skipping the boot screens costs me nothing elsewhere.
14. As a player, I want B on the main menu to do what it does today, so that there is no new way to land on a removed screen.
15. As a developer, I want the docs not to tell me to look for "- T" on a title screen that no longer shows.
16. As a developer, I want `boot-to-main-menu` and `boot-new-save-to-main-menu` to need no skip presses, so that the prefixes get shorter and every scenario boots sooner.
17. As a developer, I want scenarios that include those prefixes to keep reaching their shots, so that the whole suite doesn't need retiming by hand.
18. As a developer, I want the matching build untouched, so that `dev.py check` still verifies a byte-identical `main.dol`.
19. As a developer, I want the behaviour behind `TRAINING_BUILD` like the other training hooks, so that upstream code stays readable.
20. As a maintainer, I want the docs that describe the boot timeline (the "start from frame 62" advice and the title-screen "- T" check) corrected, so that they don't mislead the next author.
21. As a maintainer, I want the unlock notices suppressed without changing what the new save holds, so that ADR-0006's "everything unlocked" is not undone.

## Implementation Decisions

- The skip is a training-build boot-flow change, not a patch to the disc's data: the scene that follows the create-save/memory-card step becomes the main menu scene instead of the opening movie and the title. The create-save prompt and its two outcomes (create, decline) keep their existing flow and then lead to the main menu.
- Entry to the main menu must set up whatever state the title screen normally leaves behind (for example the "from title" entry mode and pad state), so the menu behaves as if reached from the title. The implementer should check what the title's Start handler sets before the main menu loads.
- The unlock notices are queued by Melee on first main-menu entry after the save changes. They are suppressed by marking them as already shown (or not queueing them) when the new save is created, in the same place that unlocks everything, so they are gone for the first launch and every later one. What the save holds (every character and stage unlocked; trophies and the feature bitmask as the new save has them) does not change.
- The notice suppression applies to the notices the unlock-all causes (random stage select, sound test, all characters, all stages, and the Kirby Hat trophy that came with them in the last measurement). Any other notice must be investigated and handled the same way if the main menu is reached with it still queued.
- The decline path (no save made) is unchanged except that it too skips the intro and title.
- The title screen's "- T" marker is gone with the title and is not replaced. Decided: this does not matter, so no substitute is drawn. The docs' "- T on the title screen" check is reworded to use the main menu.
- The scenario prefixes `boot-to-main-menu` and `boot-new-save-to-main-menu` lose their intro and title `start` presses and the eight notice-dismissing `a` presses. The main menu is ready for input a fixed number of frames after power-on (to be measured, with the existing 10-frame load margin), after the create-save prompt on the new-save variant. Every scenario that includes either prefix keeps working because it ends on the main menu with the cursor on RTC 101; only absolute frame numbers in the prefixes change.
- All scenarios are re-run and retimed as needed, not just the two prefixes. Most share the root prefixes, so most should follow once those are fixed; the rest are fixed individually.
- Other scenarios that spell out their own boot (`new-save-*`) are updated to the shorter flow. Scenarios that tested the title screen itself (`e2e-title`, the title marker check) are changed to test the main menu or are retired with a note, as the title is no longer reachable.
- Docs: the developer workflow's boot-timeline text (the `start` from frame 62 and the title-screen `start`, the "- T on the title screen" check, `shot`'s attract-loop note and the memory-card section's notice-clearing text) and the glossary are updated. No new term is needed; "main menu" and "title screen" are used as already defined.
- No ADR is required, but ADR-0007 (the training build owns the main menu) gets a one-line consequence: the build also boots straight to it.

## Testing Decisions

- A good test checks external behaviour: what screen the player is on and what the rep log says, not which scene function ran.
- Seam 1 (the highest, and an existing one): scenarios played as movies from power-on through `run --scenario`, asserted by shots and the rep log. Covered cases:
  - Fresh save: against a blank card, the create-save prompt is answered, and the main menu is on screen with no notice and the cursor on RTC 101, with no unlock-notice presses in the scenario. A press of `a` right after the menu appears does open RTC 101 (proving no notice swallowed it).
  - Existing save (the memory card snapshot): power-on to the main menu with no input at all.
  - Declined save: the decline path lands on the main menu with nothing queued.
  - No card: the "Continue without saving?" prompt still shows, then the main menu.
  - Second launch against the card the fresh-save run left (`--keep-card`): still no notices.
  - Every character and stage is still selectable (existing `new-save-character-select` and `new-save-stage-select` keep passing).
- Seam 2 (existing, the Python unit tests under `tools/dev_tests`): a structural test over the committed boot prefixes that they contain no `start` presses before the main menu, and that every scenario still expands cleanly (the existing scenario expansion tests). This is the only host-side check for this change: there is no host-side harness for the game's C code, and building one is out of scope.
- Prior art: `new-save-character-select`, `new-save-stage-select`, `new-save-declined-history`, `boot-to-main-menu`, `main-menu-rtc-101`, and the `--no-card` and `--keep-card` flags of `run`. Dolphin windows opened by these runs are closed with `dev.py stop`.
- `dev.py check` must pass: the matching build stays byte-identical.

## Out of Scope

- Skipping or changing the create-save prompt, the no-card prompt, or Melee's other boot-time prompts.
- Any change to what the new save holds (ADR-0006 stands), or to the trophy and collectible data.
- Changing the main menu, its four items, its art or the description box.
- Removing the intro movie or title screen assets from the disc (no data is removed; they just aren't reached).
- Faster boot overall: logo and load time before the menu are untouched.
- The matching build.
- Attract-loop demos: they go away with the title; nothing replaces them.
- Replacing the "- T" marker.

## Further Notes

- The attract-loop demos disappear too (confirmed by the user), as they hang off the title scene.
- Measured frames in the old prefixes (title `start` from frame 62, menu input from 30 frames after the title's `start`) will no longer apply; re-measure and keep the 10-frame margin.
- Per the repo's rules: run `dev.py stop` when finished with any Dolphin, and ask before any `dev.py run`.
