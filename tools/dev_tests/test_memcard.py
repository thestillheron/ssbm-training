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

    def test_live_runs_use_dolphins_own_card(self):
        _r, args = self.launch("--live")
        self.assertIsNone(self.card_override(args), f"args were {args}")


if __name__ == "__main__":
    unittest.main()
