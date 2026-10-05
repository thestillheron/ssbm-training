import json
import os
import shlex
import stat
import tempfile
import time
import unittest
from pathlib import Path

from .support import REPO_ROOT, dev, find_disc, requires_disc, sha1

CONFIG = REPO_ROOT / "dev.config.json"
ORIG = REPO_ROOT / "orig" / "GALE01"


def make_fake_dolphin(folder):
    """An executable that records its arguments, one per line, to args.txt."""
    folder = Path(folder)
    out = folder / "args.txt"
    if os.name == "nt":
        exe = folder / "fake_dolphin.bat"
        exe.write_text(
            "@echo off\r\n" f'(for %%a in (%*) do echo %%~a) > "{out}"\r\n'
        )
    else:
        exe = folder / "fake_dolphin.sh"
        exe.write_text(f'#!/bin/sh\nprintf "%s\\n" "$@" > {shlex.quote(str(out))}\n')
        exe.chmod(exe.stat().st_mode | stat.S_IEXEC)
    return exe, out


def original_fingerprint():
    return {
        p.relative_to(ORIG).as_posix(): sha1(p)
        for sub in ("sys", "files")
        for p in sorted((ORIG / sub).rglob("*"))
        if p.is_file() and p.name != ".gitkeep"
    }


class RunTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(prefix="ssbm-fake-dolphin-")
        self.addCleanup(self.tmp.cleanup)
        self.old_config = CONFIG.read_text() if CONFIG.exists() else None
        self.addCleanup(self.restore_config)

    def restore_config(self):
        if self.old_config is None:
            CONFIG.unlink(missing_ok=True)
        else:
            CONFIG.write_text(self.old_config)

    def configure_dolphin(self, path):
        cfg = json.loads(self.old_config) if self.old_config else {}
        cfg["dolphin"] = str(path)
        CONFIG.write_text(json.dumps(cfg))

    def test_run_without_dolphin_explains_how_to_set_it(self):
        self.configure_dolphin(Path(self.tmp.name) / "no-such-dolphin")
        r = dev(REPO_ROOT, "run")
        out = r.stdout + r.stderr
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("dev.config.json", out)
        self.assertIn("dolphin", out.lower())

    @requires_disc
    def test_run_assembles_training_game_and_launches_dolphin(self):
        r = dev(REPO_ROOT, "setup", str(find_disc()))
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        before = original_fingerprint()
        exe, args_file = make_fake_dolphin(self.tmp.name)
        self.configure_dolphin(exe)

        r = dev(REPO_ROOT, "run")
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)

        game = REPO_ROOT / "build" / "training" / "game"
        game_dol = game / "sys" / "main.dol"
        training_dol = REPO_ROOT / "build" / "training" / "main.dol"
        self.assertTrue(game_dol.is_file())
        self.assertEqual(sha1(game_dol), sha1(training_dol))
        # Dolphin got the assembled game's main.dol.
        deadline = time.time() + 20
        while time.time() < deadline and not args_file.exists():
            time.sleep(0.2)
        self.assertTrue(args_file.exists(), "fake Dolphin was never launched")
        time.sleep(0.5)
        launched = args_file.read_text().split("\n")
        self.assertTrue(
            any(a and Path(a) == game_dol for a in launched), f"args were {launched}"
        )
        # The rest of the disc is mirrored into the game folder.
        self.assertTrue((game / "sys" / "boot.bin").is_file())
        self.assertEqual(
            len([p for p in (game / "files").rglob("*") if p.name != ".gitkeep"]),
            len([p for p in (ORIG / "files").rglob("*") if p.name != ".gitkeep"]),
        )
        # The extracted original files were never written to.
        self.assertEqual(original_fingerprint(), before)

        # Running again works on an existing game folder and still leaves the
        # originals alone.
        r = dev(REPO_ROOT, "run")
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        self.assertEqual(original_fingerprint(), before)


if __name__ == "__main__":
    unittest.main()
