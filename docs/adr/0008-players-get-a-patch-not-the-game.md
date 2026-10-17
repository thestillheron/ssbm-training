# Players get a patch, not the game

Players get [RTC Training](../glossary.md#glossary_rtc_training) as a [release patch](../glossary.md#glossary_release_patch): an xdelta file in a zip on the fork's GitHub Releases, with `xdelta3.exe`, a Windows script to drag a vanilla Melee ISO onto, and a shell script for macOS and Linux. Applying it to a plain `GALE01` 1.02 ISO gives the RTC Training ISO. We never distribute the game, only the difference from it, as [Training Mode - Community Edition](https://github.com/AlexanderHarrison/TrainingMode-CommunityEdition) does. Developers build the same ISO from source with `dev.py iso`, and `dev.py release` makes the zip locally; there is no CI (ADR-0001's matching `main.dol` can't be stored publicly), so a maintainer uploads it by hand.

## Considered Options

- **Distribute the built ISO:** simplest for players, but it is the whole game, which we can't publish.
- **Build from source only:** nothing to host, but every player needs Python, this repo, a decomp toolchain download and (on macOS) Wine, just to play.
- **Ship our changed files and an installer that rebuilds the disc:** smaller than a patch, but needs our own installer on every OS, where xdelta already exists everywhere and players of TM-CE know the drag-and-drop flow.

## Consequences

- The patch applies only to a byte-exact plain ISO. RVZ must be converted to ISO first (Dolphin does this losslessly); NKit images won't work. JP and other versions aren't supported.
- Every release needs the developer's own vanilla ISO to encode against, and the release build leaves out the [authoring tools](../glossary.md#glossary_authoring_tools).
- The disc header carries our game ID (`GTCE01`, ADR-0006) and title, so Dolphin lists RTC Training apart from Melee.
