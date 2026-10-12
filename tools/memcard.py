"""Reading a raw GameCube memory card file (Dolphin's .raw), and the training
build's game ID, which the card's saves are keyed by.

Only the directory is read: enough to tell which games have a save on the card.
"""

from pathlib import Path

# The training build's own game ID (ADR-0006), written into its game folder's
# disc header and its movies so its saves never touch the player's vanilla
# Melee save.
TRAINING_GAME_ID = "GTCE01"

# 8 KiB blocks, block 0 the header, blocks 1 and 2 the two copies of the
# directory (the one with the higher update counter is current): 127 entries of
# 0x40 bytes, game code at +0 and maker code at +4, the counter at 0x1FFA.
CARD_BLOCK = 0x2000
CARD_DIR_ENTRIES = 127
CARD_DIR_ENTRY_SIZE = 0x40
CARD_DIR_COUNTER = 0x1FFA


class NotAMemoryCard(Exception):
    """The file is not a memory card Melee could have formatted (too small, or
    an empty directory block)."""


def card_game_ids(card):
    """The game IDs (game code + maker code, e.g. "GALE01") of the saves in the
    raw memory card file `card`. Raises NotAMemoryCard if it is not a memory
    card."""
    data = Path(card).read_bytes()
    if len(data) < 3 * CARD_BLOCK:
        raise NotAMemoryCard(f"{card} is too small to be a memory card")
    dirs = [data[n * CARD_BLOCK : (n + 1) * CARD_BLOCK] for n in (1, 2)]
    current = max(
        dirs,
        key=lambda d: int.from_bytes(d[CARD_DIR_COUNTER : CARD_DIR_COUNTER + 2], "big"),
    )
    if current == bytes(CARD_BLOCK) or current == b"\xff" * CARD_BLOCK:
        raise NotAMemoryCard(f"{card} has no memory card directory")
    ids = set()
    for n in range(CARD_DIR_ENTRIES):
        entry = current[n * CARD_DIR_ENTRY_SIZE : (n + 1) * CARD_DIR_ENTRY_SIZE]
        if entry[:4] != b"\xff\xff\xff\xff":
            ids.add(entry[:6].decode("ascii", "replace"))
    return ids
