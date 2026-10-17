"""`dev.py iso`: the RTC Training ISO, built in a sandbox copy of the repo.

The sandbox is set up once and `iso` run once for the module, so the real
worktree is never touched. Only external results are checked: the ISO on
disk, what dtk reports about it, and the extracted disc's files.
"""

import os
import shutil
import struct
import subprocess
import unittest

from .support import (
    REPO_ROOT,
    banner_pixels,
    banner_text_fields,
    dev,
    find_disc,
    make_full_sandbox,
    png_pixels,
    requires_disc,
    sha1,
)

ISO = ("build", "training", "rtc-training.iso")
BANNER = REPO_ROOT / "assets" / "banner"


def read_disc_file(iso, name):
    """The bytes of the file called `name` in a GameCube disc image, found
    through the disc's own file system table (boot.bin's FST offset)."""
    with open(iso, "rb") as f:
        f.seek(0x424)
        fst_offset, fst_size = struct.unpack(">II", f.read(8))
        f.seek(fst_offset)
        fst = f.read(fst_size)
        count = struct.unpack_from(">I", fst, 8)[0]
        names = fst[count * 12 :]
        for i in range(1, count):
            word, offset, size = struct.unpack_from(">III", fst, i * 12)
            if word >> 24:
                continue  # a directory
            start = word & 0xFFFFFF
            if names[start : names.index(b"\0", start)].decode("ascii") == name:
                f.seek(offset)
                return f.read(size)
    raise AssertionError(f"{name} is not on the disc")


def snapshot(folder):
    """{relative path: sha1} of every file under `folder`."""
    return {
        p.relative_to(folder).as_posix(): sha1(p)
        for p in sorted(folder.rglob("*"))
        if p.is_file()
    }


@requires_disc
class IsoTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.root = make_full_sandbox()
        r = dev(cls.root, "setup", str(find_disc()))
        if r.returncode != 0:
            raise RuntimeError(r.stdout + r.stderr)
        cls.orig_before = snapshot(cls.root / "orig" / "GALE01")
        cls.first = dev(cls.root, "iso")

    @classmethod
    def tearDownClass(cls):
        shutil.rmtree(cls.root, ignore_errors=True)

    def iso(self):
        return self.root.joinpath(*ISO)

    def disc_info(self):
        dtk = self.root / "build" / "tools" / ("dtk.exe" if os.name == "nt" else "dtk")
        r = subprocess.run(
            [str(dtk), "disc", "info", str(self.iso())], capture_output=True, text=True
        )
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        return r.stdout + r.stderr

    def test_iso_is_written_to_the_training_build_directory(self):
        self.assertEqual(self.first.returncode, 0, self.first.stdout + self.first.stderr)
        self.assertTrue(self.iso().is_file())

    def test_first_iso_downloads_gc_fst(self):
        self.assertIn("gc_fst", self.first.stdout)
        self.assertRegex(self.first.stdout, r"Downloading .*gc_fst")

    def test_iso_header_says_gtce01_and_rtc_training(self):
        info = self.disc_info()
        self.assertRegex(info, r"(?m)^Game ID: GTCE01\s*$")
        self.assertRegex(info, r"(?m)^Title: RTC Training\s*$")

    def test_extracted_disc_is_unchanged(self):
        self.assertEqual(self.first.returncode, 0, self.first.stdout + self.first.stderr)
        self.assertEqual(snapshot(self.root / "orig" / "GALE01"), self.orig_before)

    def test_iso_banner_is_ours(self):
        self.assertEqual(self.first.returncode, 0, self.first.stdout + self.first.stderr)
        bnr = read_disc_file(self.iso(), "opening.bnr")
        vanilla = (self.root / "orig" / "GALE01" / "files" / "opening.bnr").read_bytes()
        self.assertNotEqual(bnr, vanilla)
        self.assertEqual(len(bnr), len(vanilla))
        self.assertEqual(bnr[:4], b"BNR1")
        lines = (BANNER / "banner.txt").read_text(encoding="utf-8").splitlines()
        self.assertEqual(
            banner_text_fields(bnr),
            {
                "short name": lines[0],
                "short maker": lines[1],
                "long name": lines[0],
                "long maker": lines[1],
                "description": "\n".join(lines[2:]),
            },
        )
        expected = png_pixels(BANNER / "banner.png")
        actual = banner_pixels(bnr)
        for y in range(32):
            for x in range(96):
                want, got = expected[y][x], actual[y][x]
                # RGB5A3 keeps 4 or 5 bits a colour and 3 bits of alpha.
                if want[3]:
                    for c in range(3):
                        self.assertLessEqual(abs(want[c] - got[c]), 9, (x, y, want, got))
                self.assertLessEqual(abs(want[3] - got[3]), 19, (x, y, want, got))

    def test_rerun_reuses_the_download(self):
        r = dev(self.root, "iso")
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        self.assertNotRegex(r.stdout, r"Downloading .*gc_fst")
        self.assertTrue(self.iso().is_file())


if __name__ == "__main__":
    unittest.main()
