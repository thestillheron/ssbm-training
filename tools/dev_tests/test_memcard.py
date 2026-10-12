"""Movie runs and the memory card: `run --scenario` plays every movie against
a fresh copy of a fixed snapshot of the developer's slot-A card, never the
real card.

The real CLI in a sandbox copy of the repo (or, for the launch itself, the
real checkout with a fake Dolphin), with `dolphin_user` pointing at a fixture
Dolphin user folder whose Dolphin.ini puts a raw card file in slot A.
"""

import shutil
import unittest
from pathlib import Path

from tools.memcard import NotAMemoryCard, card_game_ids, card_save_data

from .support import (
    CARD_BYTES,
    CONFIG,
    PAD_INI,
    REPO_ROOT,
    RunScenarioCase,
    dev,
    fixture_card,
    make_run_sandbox,
    sha1,
    update_config,
    write_dolphin_ini,
)


class SnapshotTests(unittest.TestCase):
    """Snapshot handling happens before the build, so a sandbox without
    setup shows it (the run then stops at the build)."""

    def sandbox(self):
        root, self.user, self.args_file = make_run_sandbox({"s": "wait:60\n"})
        self.addCleanup(shutil.rmtree, root, ignore_errors=True)
        return root

    def snapshot(self, root):
        return root / "build" / "memcard" / "snapshot.USA.raw"

    def test_first_movie_run_snapshots_the_slot_a_card(self):
        root = self.sandbox()
        card = fixture_card(self.user)
        write_dolphin_ini(self.user, card=card)

        r = dev(root, "run", "--scenario", "s")
        out = r.stdout + r.stderr

        snap = self.snapshot(root)
        self.assertTrue(snap.is_file(), out)
        self.assertEqual(snap.read_bytes(), CARD_BYTES)
        self.assertEqual(card.read_bytes(), CARD_BYTES)
        # Says where the snapshot is and how to refresh it.
        self.assertIn(str(snap), out)
        self.assertIn("snapshot-card", out)
        # Then carries on to the build (no setup in the sandbox).
        self.assertIn("setup", out)

    def test_an_existing_snapshot_is_kept(self):
        root = self.sandbox()
        card = fixture_card(self.user)
        write_dolphin_ini(self.user, card=card)
        dev(root, "run", "--scenario", "s")
        # Melee (or the developer's own play) changes the real card later.
        card.write_bytes(b"changed" * 100)

        r = dev(root, "run", "--scenario", "s")

        self.assertEqual(self.snapshot(root).read_bytes(), CARD_BYTES)
        self.assertNotIn("snapshot-card", r.stdout + r.stderr)

    def test_snapshot_card_refreshes_the_snapshot(self):
        root = self.sandbox()
        card = fixture_card(self.user)
        write_dolphin_ini(self.user, card=card)
        dev(root, "run", "--scenario", "s")
        card.write_bytes(b"more unlocks" * 100)

        r = dev(root, "snapshot-card")
        out = r.stdout + r.stderr

        self.assertEqual(r.returncode, 0, out)
        self.assertEqual(self.snapshot(root).read_bytes(), b"more unlocks" * 100)
        self.assertEqual(card.read_bytes(), b"more unlocks" * 100)
        self.assertIn(str(self.snapshot(root)), out)

    def assert_card_error(self, root, *expected):
        for args in (("run", "--scenario", "s"), ("snapshot-card",)):
            r = dev(root, *args)
            out = r.stdout + r.stderr
            self.assertNotEqual(r.returncode, 0, out)
            self.assertNotIn("Building", out)
            self.assertFalse(self.args_file.exists(), "Dolphin should not have been launched")
            self.assertFalse(self.snapshot(root).exists())
            self.assertIn("slot A", out)
            for text in expected:
                self.assertIn(text, out)

    def test_a_gci_folder_in_slot_a_is_a_clear_error(self):
        root = self.sandbox()
        write_dolphin_ini(self.user, slot_a="8", card=fixture_card(self.user))
        self.assert_card_error(root, "GCI folder")

    def test_no_card_in_slot_a_is_a_clear_error(self):
        root = self.sandbox()
        write_dolphin_ini(self.user, slot_a="255")
        self.assert_card_error(root)

    def test_a_missing_card_file_is_a_clear_error(self):
        root = self.sandbox()
        missing = self.user / "GC" / "nothing-here.USA.raw"
        write_dolphin_ini(self.user, card=missing)
        self.assert_card_error(root, str(missing))

    def test_no_dolphin_ini_is_a_clear_error(self):
        root = self.sandbox()
        self.assert_card_error(root, "Dolphin.ini")

    def test_a_card_path_without_a_region_gets_dolphins_region_suffix(self):
        # Dolphin inserts the game's region before the extension when the
        # configured file name has none: card.raw -> card.USA.raw for GALE01.
        root = self.sandbox()
        fixture_card(self.user)  # GC/ssbm.USA.raw
        write_dolphin_ini(self.user, card=self.user / "GC" / "ssbm.raw")
        r = dev(root, "snapshot-card")
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        self.assertEqual(self.snapshot(root).read_bytes(), CARD_BYTES)

    def test_no_card_path_means_dolphins_default_card(self):
        root = self.sandbox()
        default = self.user / "GC" / "MemoryCardA.USA.raw"
        default.parent.mkdir(parents=True)
        default.write_bytes(CARD_BYTES)
        write_dolphin_ini(self.user, card=None)
        r = dev(root, "snapshot-card")
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        self.assertEqual(self.snapshot(root).read_bytes(), CARD_BYTES)

    def test_live_runs_ignore_the_card(self):
        # Keyboard playback keeps using Dolphin's own card: no snapshot, and
        # no error even when slot A couldn't be snapshotted.
        root = self.sandbox()
        write_dolphin_ini(self.user, slot_a="8")
        r = dev(root, "run", "--scenario", "s", "--live")
        out = r.stdout + r.stderr
        self.assertFalse(self.snapshot(root).exists())
        self.assertNotIn("slot A", out)
        self.assertIn("setup", out)

    def test_snapshot_card_works_with_no_snapshot_yet(self):
        root = self.sandbox()
        card = fixture_card(self.user)
        write_dolphin_ini(self.user, card=card)

        r = dev(root, "snapshot-card")

        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        self.assertEqual(self.snapshot(root).read_bytes(), CARD_BYTES)


class CardGameIdTests(unittest.TestCase):
    """Which games have a save on a raw memory card (ADR-0006: the snapshot
    needs one for GTCE01)."""

    def card(self, ids, counters=(1, 0)):
        """A card image whose directory block 1 lists `ids` (saves) and
        block 2 is an older copy with none, counter per `counters`."""
        block = 0x2000
        data = bytearray(b"\xff" * (3 * block))
        for n, counter in zip((1, 2), counters):
            d = bytearray(b"\xff" * block)
            if n == 1:
                for i, gid in enumerate(ids):
                    d[i * 0x40 : i * 0x40 + 6] = gid.encode()
            d[0x1FFA:0x1FFC] = counter.to_bytes(2, "big")
            data[n * block : (n + 1) * block] = d
        path = Path(self.tmp) / "card.raw"
        path.write_bytes(bytes(data))
        return path

    def setUp(self):
        import tempfile

        self.tmpdir = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmpdir.cleanup)
        self.tmp = self.tmpdir.name

    def test_lists_the_game_ids_of_the_saves_on_the_card(self):
        card = self.card(["GALE01", "GTCE01"])
        self.assertEqual(card_game_ids(card), {"GALE01", "GTCE01"})

    def test_reads_the_directory_copy_with_the_higher_update_counter(self):
        card = self.card(["GALE01"], counters=(1, 2))  # block 2 (empty) is current
        self.assertEqual(card_game_ids(card), set())

    def test_a_file_that_is_not_a_memory_card_is_reported_as_such(self):
        with self.assertRaises(NotAMemoryCard):
            card_game_ids(self.card_from(CARD_BYTES))
        with self.assertRaises(NotAMemoryCard):
            card_game_ids(self.card_from(b""))

    def card_from(self, data):
        path = Path(self.tmp) / "other.raw"
        path.write_bytes(data)
        return path


class CardSaveDataTests(unittest.TestCase):
    """The bytes of a game's save on a raw card (its file's block chain), which
    tells whether a run left another game's save alone."""

    def setUp(self):
        import tempfile

        self.tmpdir = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmpdir.cleanup)
        self.card = Path(self.tmpdir.name) / "card.raw"

    def make_card(self, files):
        """A card with a file per (game id, block chain, data); a chain's
        blocks may be anywhere from block 5 on."""
        import struct

        block = 0x2000
        data = bytearray(b"\x00" * (12 * block))
        directory = bytearray(b"\xff" * block)
        fat = bytearray(block)
        for n, (game_id, chain, content) in enumerate(files):
            entry = bytearray(0x40)
            entry[:6] = game_id.encode()
            entry[0x36:0x38] = struct.pack(">H", chain[0])
            entry[0x38:0x3A] = struct.pack(">H", len(chain))
            directory[n * 0x40 : (n + 1) * 0x40] = entry
            for i, b in enumerate(chain):
                nxt = chain[i + 1] if i + 1 < len(chain) else 0xFFFF
                fat[0xA + (b - 5) * 2 : 0xC + (b - 5) * 2] = struct.pack(">H", nxt)
                chunk = content[i * block : (i + 1) * block]
                data[b * block : b * block + len(chunk)] = chunk
        directory[0x1FFA:0x1FFC] = (2).to_bytes(2, "big")
        data[1 * block : 2 * block] = directory
        older = bytearray(b"\xff" * block)  # the older, empty copy
        older[0x1FFA:0x1FFC] = (1).to_bytes(2, "big")
        data[2 * block : 3 * block] = older
        fat[4:6] = (0).to_bytes(2, "big")
        data[3 * block : 4 * block] = fat
        self.card.write_bytes(bytes(data))
        return self.card

    def test_a_games_save_is_its_files_blocks_in_chain_order(self):
        content = bytes(range(256)) * 64  # 0x4000 bytes: two blocks
        card = self.make_card([("GALE01", [5], b"vanilla" * 100), ("GTCE01", [9, 6], content)])
        self.assertEqual(card_save_data(card, "GTCE01"), content)
        self.assertEqual(card_save_data(card, "GALE01")[:7], b"vanilla")
        self.assertIsNone(card_save_data(card, "GXXE01"))


class MovieLaunchCardTests(RunScenarioCase):
    """The launch itself, in the real checkout (it builds), with a fake
    Dolphin that never opens a game window and a fixture Dolphin user folder.
    The checkout's own snapshot is set aside and restored."""

    def setUp(self):
        super().setUp()
        self.memcard_dir = REPO_ROOT / "build" / "memcard"
        saved = Path(self.tmp.name) / "saved-memcard"
        if self.memcard_dir.exists():
            shutil.copytree(self.memcard_dir, saved)
        self.addCleanup(self.restore_memcard_dir, saved)

        self.user = Path(self.tmp.name) / "user"
        self.card = fixture_card(self.user, b"real card" * 100)
        write_dolphin_ini(self.user, card=self.card)
        (self.user / "Config" / "GCPadNew.ini").write_text(PAD_INI)
        update_config(CONFIG, dolphin_user=str(self.user))
        self.snapshot = self.memcard_dir / "snapshot.USA.raw"
        self.snapshot.parent.mkdir(parents=True, exist_ok=True)
        self.snapshot.write_bytes(CARD_BYTES)

    def restore_memcard_dir(self, saved):
        shutil.rmtree(self.memcard_dir, ignore_errors=True)
        if saved.exists():
            shutil.copytree(saved, self.memcard_dir)

    def launch(self, *args):
        self.args_file.unlink(missing_ok=True)
        r = dev(REPO_ROOT, "run", "--scenario", "e2e-smoke", *args)
        return r, self.launched_args()

    def card_override(self, args):
        """The slot-A card path given with `-C Dolphin.Core.MemcardAPath=...`."""
        prefix = "Dolphin.Core.MemcardAPath="
        found = [a for a in args if a.startswith(prefix)]
        if not found:
            return None
        self.assertEqual(args[args.index(found[0]) - 1], "-C", args)
        return Path(found[0][len(prefix):])

    def test_movie_runs_play_against_a_fresh_copy_of_the_snapshot(self):
        real_card = sha1(self.card)
        _r, args = self.launch()
        run_copy = self.card_override(args)
        self.assertIsNotNone(run_copy, f"args were {args}")
        self.assertTrue(run_copy.is_relative_to(REPO_ROOT / "build"), run_copy)
        # Dolphin inserts a region into a card name without one.
        self.assertTrue(run_copy.name.endswith(".USA.raw"), run_copy)
        self.assertNotEqual(run_copy, self.snapshot)
        self.assertEqual(run_copy.read_bytes(), CARD_BYTES)
        # Slot A is a raw card file for this launch, whatever Dolphin.ini says.
        self.assertIn("Dolphin.Core.SlotA=1", args)

        # Melee writes the run copy during play; the next run starts afresh.
        run_copy.write_bytes(b"saved during the run" * 100)
        _r, args = self.launch()
        self.assertEqual(self.card_override(args).read_bytes(), CARD_BYTES)
        self.assertEqual(self.snapshot.read_bytes(), CARD_BYTES)
        self.assertEqual(sha1(self.card), real_card)

    def test_keep_card_plays_against_the_run_copy_the_last_run_left(self):
        # With nothing left behind it starts from the snapshot as usual.
        (self.memcard_dir / "run.USA.raw").unlink(missing_ok=True)
        _r, args = self.launch("--keep-card")
        run_copy = self.card_override(args)
        self.assertEqual(run_copy.read_bytes(), CARD_BYTES)

        # Melee saved during that run; the next one carries on from it.
        run_copy.write_bytes(b"saved during the run" * 100)
        _r, args = self.launch("--keep-card")
        self.assertEqual(self.card_override(args).read_bytes(), b"saved during the run" * 100)
        self.assertEqual(self.snapshot.read_bytes(), CARD_BYTES)

        # Without the flag the run copy is replaced, as before.
        _r, args = self.launch()
        self.assertEqual(self.card_override(args).read_bytes(), CARD_BYTES)

    def test_no_card_boots_with_nothing_in_slot_a_and_leaves_the_run_copy_alone(self):
        self.launch()
        run_copy = self.memcard_dir / "run.USA.raw"
        run_copy.write_bytes(b"left by the last run" * 100)

        _r, args = self.launch("--no-card")

        self.assertIn("Dolphin.Core.SlotA=255", args)
        self.assertIsNone(self.card_override(args), f"args were {args}")
        self.assertEqual(run_copy.read_bytes(), b"left by the last run" * 100)

    def test_live_runs_use_dolphins_own_card(self):
        _r, args = self.launch("--live")
        self.assertIsNone(self.card_override(args), f"args were {args}")


if __name__ == "__main__":
    unittest.main()
