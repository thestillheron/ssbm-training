# The training build has its own game ID

Amends [ADR-0005](0005-drills-are-event-match-events.md), which had the training build share the player's real save.

Melee finds its save on the memory card by the disc's game code and company plus one fixed file name (`SuperSmashBros0110290334`), and Dolphin files saves by the same game ID. So any GALE01 disc, ours included, reads and writes the player's vanilla Melee save. The training build writes a game ID of its own into the disc header, as UnclePunch's training pack does (`GTME01`), so every save it makes, its own drill [history](../glossary.md#glossary_history) and Melee's normal save data alike, goes to a separate file and can never overwrite another Melee save. Because a new ID starts from a blank save, the training build unlocks every character and stage when it creates one.

## Considered Options

- **Keep GALE01 and keep our data in a card file with its own name:** our file is safe, but the build still writes the player's vanilla save through Melee's normal save points.
- **Keep GALE01 and keep our data in unused padding in the vanilla save:** puts our bytes in the player's real save, which vanilla is only inferred, never tested, to ignore.

## Consequences

- Dolphin's per-game settings and codes for GALE01 don't apply to the training build.
- The memory card snapshot that scenario movies play against needs a save under the new ID.
