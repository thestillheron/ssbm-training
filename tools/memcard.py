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


# The two copies of the block allocation table are blocks 3 and 4: u16
# checksum, inverse checksum, update counter, free blocks, last allocated,
# then the next block of each block from block 5 on (0xFFFF ends a chain).
CARD_FAT_FIRST_BLOCK = 5
CARD_FAT_MAP = 0xA
CARD_FAT_COUNTER = 4
CARD_ENTRY_FIRST_BLOCK = 0x36
CARD_ENTRY_BLOCK_COUNT = 0x38


def _current_directory(data):
    dirs = [data[n * CARD_BLOCK : (n + 1) * CARD_BLOCK] for n in (1, 2)]
    return max(
        dirs,
        key=lambda d: int.from_bytes(d[CARD_DIR_COUNTER : CARD_DIR_COUNTER + 2], "big"),
    )


def _current_fat(data):
    fats = [data[n * CARD_BLOCK : (n + 1) * CARD_BLOCK] for n in (3, 4)]
    return max(
        fats,
        key=lambda f: int.from_bytes(f[CARD_FAT_COUNTER : CARD_FAT_COUNTER + 2], "big"),
    )


def card_save_data(card, game_id):
    """The bytes of the first save for `game_id` on the raw memory card file
    `card` (its file's blocks in chain order), or None when it has none."""
    data = Path(card).read_bytes()
    directory = _current_directory(data)
    fat = _current_fat(data)
    for n in range(CARD_DIR_ENTRIES):
        entry = directory[n * CARD_DIR_ENTRY_SIZE : (n + 1) * CARD_DIR_ENTRY_SIZE]
        if entry[:6].decode("ascii", "replace") != game_id:
            continue
        block = int.from_bytes(entry[CARD_ENTRY_FIRST_BLOCK : CARD_ENTRY_FIRST_BLOCK + 2], "big")
        count = int.from_bytes(entry[CARD_ENTRY_BLOCK_COUNT : CARD_ENTRY_BLOCK_COUNT + 2], "big")
        out = bytearray()
        for _ in range(count):
            if block < CARD_FAT_FIRST_BLOCK or (block + 1) * CARD_BLOCK > len(data):
                break
            out += data[block * CARD_BLOCK : (block + 1) * CARD_BLOCK]
            at = CARD_FAT_MAP + (block - CARD_FAT_FIRST_BLOCK) * 2
            block = int.from_bytes(fat[at : at + 2], "big")
        return bytes(out)
    return None
