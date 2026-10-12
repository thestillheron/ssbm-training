"""Stable drill IDs (drill-history ticket 01): history is keyed by a drill's ID,
not its table index, so every record in the drill table must carry its own.

Reads the sources only (src/training/drill.h's DrillId enum and the table in
src/training/drill.c), so it needs no build.
"""

import re
import unittest

from .support import REPO_ROOT

TRAINING = REPO_ROOT / "src" / "training"


def declared_ids():
    """The (name, value) pairs of the DrillId enum in drill.h."""
    text = (TRAINING / "drill.h").read_text(encoding="utf-8")
    body = re.search(r"typedef enum DrillId \{(.*?)\} DrillId;", text, re.S)
    if body is None:
        return []
    return [(m.group(1), int(m.group(2)))
            for m in re.finditer(r"(DrillId_\w+)\s*=\s*(\d+)", body.group(1))]


def table_entries():
    """The source text of each record in the drill table (not the Custom
    drill's), one per entry: from its opening brace at the entry indent to the
    closing one."""
    text = (TRAINING / "drill.c").read_text(encoding="utf-8")
    table = re.search(r"static const Drill drills\[\] = \{(.*?)\n\};", text, re.S)
    return re.findall(r"^    \{\n(.*?)^    \},", table.group(1), re.S | re.M)


class DrillIdTests(unittest.TestCase):
    def test_every_table_drill_names_one_id(self):
        entries = table_entries()
        self.assertGreater(len(entries), 8)
        for entry in entries:
            name = entry.splitlines()[0].strip()
            self.assertEqual(len(re.findall(r"DrillId_\w+", entry)), 1,
                             f"{name}: needs exactly one DrillId_")

    def test_ids_are_distinct_and_nonzero(self):
        ids = declared_ids()
        self.assertGreater(len(ids), 8)
        values = [v for _, v in ids]
        self.assertEqual(len(values), len(set(values)), f"duplicate value in {ids}")
        self.assertNotIn(0, [v for n, v in ids if n != "DrillId_None"])

    def test_no_table_drill_shares_an_id_with_another(self):
        used = []
        for entry in table_entries():
            used += re.findall(r"DrillId_\w+", entry)
        self.assertEqual(len(used), len(set(used)), f"an ID is used twice: {used}")
        values = dict(declared_ids())
        for name in used:
            self.assertIn(name, values, f"{name} is not declared in drill.h")


if __name__ == "__main__":
    unittest.main()
