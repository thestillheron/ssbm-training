"""Character drop heights (character-drop-heights ticket 01): the per-character
table in src/training/drop_height.c must have a row for every playable
character. Values are not checked: the developer tunes them by hand.

Reads the sources only, so it needs no build.
"""

import re
import unittest

from .support import REPO_ROOT

# FighterKinds the opponent cannot be: Nana (Ice Climbers use Popo's row), the
# bosses, the wireframes, Giga Bowser and Sandbag, plus the terminator.
NOT_PLAYABLE = {
    "Ft_Kind_Nana", "Ft_Kind_MasterH", "Ft_Kind_CrezyH", "Ft_Kind_Boy",
    "Ft_Kind_Girl", "Ft_Kind_GKoops", "Ft_Kind_Sandbag", "Ft_Kind_None",
    "Ft_Kind_Max",
}


def fighter_kinds():
    text = (REPO_ROOT / "src" / "melee" / "ft" / "forward.h").read_text(encoding="utf-8")
    body = re.search(r"typedef enum FighterKind \{(.*?)\} FighterKind;", text, re.S)
    return re.findall(r"(Ft_Kind_\w+)", body.group(1))


def table_rows():
    text = (REPO_ROOT / "src" / "training" / "drop_height.c").read_text(encoding="utf-8")
    table = re.search(r"drop_heights\[\] = \{(.*?)\n\};", text, re.S)
    return re.findall(r"\{\s*(Ft_Kind_\w+),\s*([\d.]+)F\s*\}", table.group(1))


class DropHeightTableTests(unittest.TestCase):
    def test_every_playable_character_has_a_row(self):
        have = [k for k, _ in table_rows()]
        want = [k for k in fighter_kinds() if k not in NOT_PLAYABLE]
        self.assertEqual(len(want), 26)
        self.assertEqual([k for k in want if k not in have], [])

    def test_no_character_has_two_rows(self):
        have = [k for k, _ in table_rows()]
        self.assertEqual(len(have), len(set(have)))
