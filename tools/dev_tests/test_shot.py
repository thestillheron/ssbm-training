"""`dev.py shot` (piloting): the screenshot hotkey from Dolphin's config, and
the routine that triggers it, waits for Dolphin's file and files it.

The CLI is driven in a sandbox with a fixture Dolphin user folder
(`shot --dry-run`, which touches no Dolphin). The live routine is driven with
a fake backend that plays the part of Dolphin by writing a PNG when the
hotkey goes down. No real Dolphin is launched.
"""

import shutil
import sys
import tempfile
import unittest
from pathlib import Path

from .support import REPO_ROOT, dev
from .test_pad import make_sandbox as make_pad_sandbox

sys.path.insert(0, str(REPO_ROOT))
from tools import pad, shot  # noqa: E402

DOLPHIN = 100
EDITOR = 300
PNG = b"\x89PNG\r\n\x1a\nfake"


def make_sandbox(hotkeys_ini=None):
    root, user = make_pad_sandbox()
    shutil.copy(REPO_ROOT / "tools" / "shot.py", root / "tools")
    if hotkeys_ini is not None:
        (user / "Config" / "Hotkeys.ini").write_text(hotkeys_ini)
    return root, user


class ShotCliTests(unittest.TestCase):
    def sandbox(self, hotkeys_ini=None):
        root, user = make_sandbox(hotkeys_ini)
        self.addCleanup(shutil.rmtree, root, ignore_errors=True)
        return root, user

    def dry(self, root, *extra):
        r = dev(root, "shot", "--dry-run", *extra)
        return r, r.stdout + r.stderr

    def test_default_hotkey_is_f9_without_a_hotkey_config(self):
        root, _ = self.sandbox()
        r, out = self.dry(root)
        self.assertEqual(r.returncode, 0, out)
        self.assertIn("F9", out)
        self.assertIn("0x43", out)

    def test_hotkey_comes_from_dolphins_hotkey_config(self):
        root, _ = self.sandbox(
            "[Hotkeys]\nDevice = DInput/0/Keyboard Mouse\nGeneral/Take Screenshot = F12\n"
        )
        r, out = self.dry(root)
        self.assertEqual(r.returncode, 0, out)
        self.assertIn("F12", out)
        self.assertIn("0x58", out)

    def test_hotkey_config_without_the_binding_falls_back_to_f9(self):
        root, _ = self.sandbox("[Hotkeys]\nGeneral/Stop = ESCAPE\n")
        r, out = self.dry(root)
        self.assertEqual(r.returncode, 0, out)
        self.assertIn("F9", out)

    def test_modifier_chord_is_a_clear_error(self):
        root, _ = self.sandbox("[Hotkeys]\nGeneral/Take Screenshot = @(Ctrl+F9)\n")
        r, out = self.dry(root)
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("Take Screenshot", out)
        self.assertIn("Ctrl+F9", out)

    def test_dry_run_names_where_the_shot_lands(self):
        root, user = self.sandbox()
        r, out = self.dry(root, "title")
        self.assertEqual(r.returncode, 0, out)
        self.assertIn(str(root / "build" / "training" / "shots" / "title.png"), out)
        self.assertIn(str(user / "ScreenShots"), out)

    def test_label_cannot_escape_the_shots_folder(self):
        root, _ = self.sandbox()
        r, out = self.dry(root, "../evil")
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("label", out)

    @unittest.skipUnless(sys.platform == "darwin", "macOS only")
    def test_macos_is_not_supported_yet(self):
        root, _ = self.sandbox()
        r = dev(root, "shot")
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("not supported yet", r.stdout + r.stderr)


class FakeDolphinBackend:
    """Foreground bookkeeping like the pad tests' fake, plus a Dolphin that
    writes a screenshot when the hotkey goes down (unless `silent`)."""

    def __init__(self, screenshots, silent=False):
        self.screenshots = Path(screenshots)
        self.silent = silent
        self.fg = EDITOR
        self.log = []
        self.count = 0

    def foreground(self):
        return self.fg

    def focus(self, hwnd):
        self.log.append(("focus", hwnd))
        self.fg = hwnd

    def key(self, key, down):
        self.log.append(("down" if down else "up", key.name))
        if down and not self.silent:
            self.count += 1
            folder = self.screenshots / "GALE01"
            folder.mkdir(parents=True, exist_ok=True)
            (folder / f"GALE01-{self.count}.png").write_bytes(PNG)


class TakeShotTests(unittest.TestCase):
    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp(prefix="ssbm-shot-"))
        self.addCleanup(shutil.rmtree, self.tmp, ignore_errors=True)
        self.screenshots = self.tmp / "ScreenShots"
        self.shots = self.tmp / "build" / "training" / "shots"
        self.key = pad.key_from_name("F9")
        self.now = 0.0

    def clock(self):
        return self.now

    def sleep(self, s):
        self.now += s

    def take(self, backend, label=None, timeout=5.0):
        return shot.take_shot(
            label,
            self.key,
            DOLPHIN,
            backend,
            self.screenshots,
            self.shots,
            timeout=timeout,
            sleep=self.sleep,
            clock=self.clock,
        )

    def test_moves_the_new_file_to_the_shots_folder_under_the_label(self):
        b = FakeDolphinBackend(self.screenshots)
        path = self.take(b, "title")
        self.assertEqual(path, (self.shots / "title.png").resolve())
        self.assertEqual(path.read_bytes(), PNG)
        self.assertEqual(list(self.screenshots.rglob("*.png")), [])
        self.assertEqual(b.log[0], ("focus", DOLPHIN))
        self.assertEqual(b.log[-1], ("focus", EDITOR))

    def test_without_a_label_the_name_is_a_timestamp(self):
        path = self.take(FakeDolphinBackend(self.screenshots))
        self.assertRegex(path.name, r"^\d{8}-\d{6}(-\d+)?\.png$")

    def test_only_a_new_file_counts_not_older_screenshots(self):
        old = self.screenshots / "GALE01"
        old.mkdir(parents=True)
        (old / "GALE01-1.png").write_bytes(b"old")
        b = FakeDolphinBackend(self.screenshots)
        b.count = 1
        path = self.take(b, "fresh")
        self.assertEqual(path.read_bytes(), PNG)
        self.assertEqual((old / "GALE01-1.png").read_bytes(), b"old")

    def test_timeout_is_a_clear_error_and_leaves_no_file(self):
        b = FakeDolphinBackend(self.screenshots, silent=True)
        with self.assertRaisesRegex(pad.PadError, "no new screenshot.*within 5"):
            self.take(b, "never", timeout=5.0)
        self.assertFalse((self.shots / "never.png").exists())
        self.assertEqual(b.log[-1], ("focus", EDITOR))

    def test_relabelling_replaces_the_earlier_shot(self):
        self.take(FakeDolphinBackend(self.screenshots), "same")
        path = self.take(FakeDolphinBackend(self.screenshots), "same")
        self.assertEqual(path.read_bytes(), PNG)

    def test_clear_shots_empties_the_folder(self):
        self.shots.mkdir(parents=True)
        (self.shots / "a.png").write_bytes(PNG)
        shot.clear_shots(self.shots)
        self.assertEqual(list(self.shots.glob("*")), [])
        shot.clear_shots(self.tmp / "missing")  # no folder is fine


if __name__ == "__main__":
    unittest.main()
