import json
import shutil
import tempfile
import time
import unittest
from pathlib import Path

from .support import (
    MAIN_DOL_SHA1,
    dev,
    find_disc,
    make_sandbox,
    requires_disc,
    reset_orig,
    sha1,
)


class SetupTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.root = make_sandbox()
        cls.tmp = Path(tempfile.mkdtemp(prefix="ssbm-dev-img-"))

    @classmethod
    def tearDownClass(cls):
        shutil.rmtree(cls.root, ignore_errors=True)
        shutil.rmtree(cls.tmp, ignore_errors=True)

    def setUp(self):
        reset_orig(self.root)

    def assertNothingExtracted(self):
        orig = self.root / "orig" / "GALE01"
        self.assertFalse((orig / "sys" / "main.dol").exists())
        self.assertEqual(list((orig / "files").iterdir()), [])
        self.assertEqual(list(orig.glob(".extract*")), [])

    def test_corrupt_image_is_rejected_without_extracting(self):
        junk = self.tmp / "junk.iso"
        junk.write_bytes(b"this is not a disc image" * 1000)
        r = dev(self.root, "setup", str(junk))
        self.assertNotEqual(r.returncode, 0, r.stdout + r.stderr)
        self.assertNothingExtracted()

    def test_no_image_argument_and_none_in_orig_fails_with_message(self):
        r = dev(self.root, "setup")
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("disc image", r.stdout + r.stderr)

    def test_several_images_in_orig_fails_naming_them(self):
        orig = self.root / "orig" / "GALE01"
        (orig / "a.iso").write_bytes(b"x")
        (orig / "b.rvz").write_bytes(b"x")
        r = dev(self.root, "setup")
        out = r.stdout + r.stderr
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("more than one", out)
        self.assertIn("a.iso", out)
        self.assertIn("b.rvz", out)
        self.assertNothingExtracted()

    def test_no_images_message_says_where_to_put_one(self):
        r = dev(self.root, "setup")
        out = r.stdout + r.stderr
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("no disc image found", out)
        self.assertIn("orig", out)

    def write_config(self, cfg):
        (self.root / "dev.config.json").write_text(json.dumps(cfg))

    def read_config(self):
        return json.loads((self.root / "dev.config.json").read_text())

    def tearDown(self):
        (self.root / "dev.config.json").unlink(missing_ok=True)

    def test_missing_dolphin_prints_install_hint_and_is_not_fatal(self):
        self.write_config({"dolphin": str(self.tmp / "nowhere" / "Dolphin")})
        r = dev(self.root, "setup")
        out = r.stdout + r.stderr
        self.assertIn("dolphin-emu.org", out)
        # Not fatal: the run goes on to the (missing) disc image instead.
        self.assertIn("no disc image found", out)

    def test_missing_wine_fails_with_install_command_and_quarantine_note(self):
        self.write_config({"wine": str(self.tmp / "nowhere" / "wine")})
        r = dev(self.root, "setup")
        out = r.stdout + r.stderr
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("brew install --cask --no-quarantine wine-stable", out)
        self.assertIn("quarantine", out)
        self.assertNotIn("no disc image found", out)

    def test_setup_records_detected_paths_without_touching_overrides(self):
        fake = self.tmp / "FakeDolphin"
        fake.write_bytes(b"")
        self.write_config({"dolphin": str(fake), "disc": "keep-me"})
        dev(self.root, "setup")
        cfg = self.read_config()
        self.assertEqual(cfg["disc"], "keep-me")
        self.assertEqual(cfg["dolphin"], str(fake))
        self.assertEqual(cfg["detected"]["dolphin"], str(fake))

    def test_missing_dolphin_is_not_recorded_as_detected(self):
        self.write_config({"dolphin": str(self.tmp / "nowhere")})
        dev(self.root, "setup")
        self.assertNotIn("dolphin", self.read_config().get("detected", {}))

    @requires_disc
    def test_setup_extracts_full_disc_and_verifies_main_dol(self):
        r = dev(self.root, "setup", str(find_disc()))
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        orig = self.root / "orig" / "GALE01"
        self.assertEqual(sha1(orig / "sys" / "main.dol"), MAIN_DOL_SHA1)
        self.assertTrue((orig / "sys" / "boot.bin").is_file())
        self.assertTrue((orig / "files" / "MvEndCaptain.mth").is_file())
        self.assertTrue((self.root / ".venv").is_dir())

    @requires_disc
    def test_rerun_is_a_fast_noop(self):
        disc = str(find_disc())
        self.assertEqual(dev(self.root, "setup", disc).returncode, 0)
        main_dol = self.root / "orig" / "GALE01" / "sys" / "main.dol"
        before = main_dol.stat().st_mtime_ns
        t = time.time()
        r = dev(self.root, "setup", disc)
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        self.assertLess(time.time() - t, 15)
        self.assertEqual(main_dol.stat().st_mtime_ns, before)

    @requires_disc
    def test_wrong_game_version_is_rejected_with_message(self):
        # A structurally valid disc whose main.dol differs from NTSC-U 1.02.
        bad = self.tmp / "other.iso"
        shutil.copy(find_disc(), bad)
        with open(bad, "r+b") as f:
            f.seek(0x420)
            dol_off = int.from_bytes(f.read(4), "big")
            f.seek(dol_off + 0x200)
            f.write(b"\xde\xad\xbe\xef")
        r = dev(self.root, "setup", str(bad))
        bad.unlink()
        self.assertNotEqual(r.returncode, 0, r.stdout + r.stderr)
        self.assertIn("GALE01", r.stdout + r.stderr)
        self.assertNothingExtracted()

    @requires_disc
    def test_disc_found_in_orig_folder_without_argument(self):
        orig = self.root / "orig" / "GALE01"
        shutil.copy(find_disc(), orig / "Melee.iso")
        r = dev(self.root, "setup")
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        self.assertEqual(sha1(orig / "sys" / "main.dol"), MAIN_DOL_SHA1)


if __name__ == "__main__":
    unittest.main()
