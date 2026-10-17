"""`dev.py release`: the release patch zip, made in a sandbox copy of the repo.

Only external results are checked: the zip and its files, what applying its
patch with xdelta3 gives, what its scripts print, and the extracted disc.
"""

import os
import shutil
import subprocess
import tempfile
import unittest
import zipfile
from pathlib import Path

from .support import dev, find_disc, make_full_sandbox, requires_disc, sha1
from .test_iso import ISO, snapshot

ZIP = ("build", "release", "rtc-training.zip")
UNVERIFIED_ZIP = ("build", "release", "rtc-training-UNVERIFIED.zip")


def modified_copy(src, dest):
    """A copy of `src` with a byte flipped every 64 MiB, so it is not vanilla."""
    shutil.copyfile(src, dest)
    size = dest.stat().st_size
    with open(dest, "r+b") as f:
        for offset in range(0x1000, size, 64 << 20):
            f.seek(offset)
            b = f.read(1)
            f.seek(offset)
            f.write(bytes([b[0] ^ 0xFF]))
    return dest


@requires_disc
class RefusesUnverifiedDiscTests(unittest.TestCase):
    """The verification gate runs before anything is built, so no setup."""

    @classmethod
    def setUpClass(cls):
        cls.root = make_full_sandbox()
        cls.bad = modified_copy(find_disc(), cls.root / "modified.iso")
        cls.result = dev(cls.root, "release", str(cls.bad))

    @classmethod
    def tearDownClass(cls):
        shutil.rmtree(cls.root, ignore_errors=True)

    def test_a_disc_that_fails_verification_is_refused(self):
        out = self.result.stdout + self.result.stderr
        self.assertNotEqual(self.result.returncode, 0, out)
        self.assertIn("verif", out.lower())

    def test_nothing_is_built_or_zipped(self):
        self.assertFalse(self.root.joinpath(*ZIP).exists())
        self.assertFalse((self.root / "build" / "training" / "rtc-training.iso").exists())


def vanilla_iso(root, dest):
    """The test disc as a plain ISO at `dest` (converted with the sandbox's
    dtk if it is not one)."""
    disc = find_disc()
    if disc.suffix.lower() == ".iso":
        shutil.copyfile(disc, dest)
    else:
        dtk = root / "build" / "tools" / ("dtk.exe" if os.name == "nt" else "dtk")
        subprocess.run([str(dtk), "disc", "convert", str(disc), str(dest)], check=True)
    return dest


def find_xdelta3(folder):
    """The xdelta3 to apply the patch with: the zip's own on Windows, else
    one on PATH (None if there is none)."""
    if os.name == "nt":
        return folder / "xdelta3.exe"
    found = shutil.which("xdelta3")
    return Path(found) if found else None


def find_bash():
    """A bash for the .sh: Git's on Windows (not WSL's), else the system's."""
    if os.name == "nt":
        git = shutil.which("git")
        if git:
            # git.exe is in Git\cmd or Git\mingw64\bin; bash is Git\bin\bash.exe.
            for bash in (Path(git).parents[n] / "bin" / "bash.exe" for n in (1, 2)):
                if bash.is_file():
                    return bash
        return None
    found = shutil.which("bash")
    return Path(found) if found else None


@requires_disc
class ReleaseTests(unittest.TestCase):
    """One release for the module, made after a build configured with the
    authoring tools and a non-default tech pool. A disc that fails Redump
    verification (a trimmed dump, say) gets a release with
    --allow-unverified-disc instead, which is otherwise the same."""

    @classmethod
    def setUpClass(cls):
        cls.root = make_full_sandbox()
        cls.tmp = Path(tempfile.mkdtemp(prefix="ssbm-release-"))
        disc = str(find_disc())
        r = dev(cls.root, "setup", disc)
        if r.returncode != 0:
            raise RuntimeError(r.stdout + r.stderr)
        cls.orig_before = snapshot(cls.root / "orig" / "GALE01")
        cls.disc_before = sha1(find_disc())
        r = dev(cls.root, "build", "--authoring", "--tech-pool", "in")
        if r.returncode != 0:
            raise RuntimeError(r.stdout + r.stderr)
        cls.result = dev(cls.root, "release", disc)
        cls.zip = cls.root.joinpath(*ZIP)
        if cls.result.returncode != 0 and "failed verification" in cls.result.stderr:
            cls.result = dev(cls.root, "release", disc, "--allow-unverified-disc")
            cls.zip = cls.root.joinpath(*UNVERIFIED_ZIP)
        if cls.result.returncode != 0:
            raise RuntimeError(cls.result.stdout + cls.result.stderr)
        cls.iso_sha1 = sha1(cls.root.joinpath(*ISO))
        with zipfile.ZipFile(cls.zip) as z:
            cls.names = sorted(z.namelist())
            cls.infos = {i.filename: i for i in z.infolist()}
            z.extractall(cls.tmp / "unzipped")
        cls.folder = cls.tmp / "unzipped" / "rtc-training"
        cls.vanilla = vanilla_iso(cls.root, cls.tmp / "vanilla.iso")
        cls.modified = modified_copy(cls.vanilla, cls.tmp / "modified.iso")

    @classmethod
    def tearDownClass(cls):
        shutil.rmtree(cls.root, ignore_errors=True)
        shutil.rmtree(cls.tmp, ignore_errors=True)

    def apply(self, source, out):
        xdelta3 = find_xdelta3(self.folder)
        if xdelta3 is None:
            self.skipTest("no xdelta3 on PATH")
        return subprocess.run(
            [str(xdelta3), "-f", "-d", "-s", str(source), str(self.folder / "patch.xdelta"), str(out)],
            capture_output=True,
            text=True,
        )

    def test_zip_holds_one_folder_with_the_patch_xdelta3_and_scripts(self):
        self.assertEqual(
            self.names,
            sorted(
                f"rtc-training/{n}"
                for n in (
                    "patch.xdelta",
                    "xdelta3.exe",
                    "DRAG VANILLA MELEE HERE.bat",
                    "build_linux_and_mac.sh",
                )
            ),
        )

    def test_shell_script_is_executable_with_unix_line_endings(self):
        info = self.infos["rtc-training/build_linux_and_mac.sh"]
        self.assertTrue((info.external_attr >> 16) & 0o111)
        self.assertNotIn(b"\r\n", (self.folder / "build_linux_and_mac.sh").read_bytes())

    def test_patch_on_vanilla_gives_the_rtc_training_iso(self):
        out = self.tmp / "patched.iso"
        r = self.apply(self.vanilla, out)
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        self.assertEqual(sha1(out), self.iso_sha1)
        out.unlink()

    def test_patch_on_a_modified_iso_fails(self):
        out = self.tmp / "patched-modified.iso"
        r = self.apply(self.modified, out)
        self.assertNotEqual(r.returncode, 0, r.stdout + r.stderr)
        out.unlink(missing_ok=True)

    def test_release_build_is_the_iso_build_without_authoring_tools(self):
        # `iso` always builds without the authoring tools and with each
        # drill's own tech pool; the release was made after a build with
        # both, so its ISO must still be what `iso` gives.
        r = dev(self.root, "iso")
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        self.assertEqual(sha1(self.root.joinpath(*ISO)), self.iso_sha1)

    @unittest.skipUnless(os.name == "nt", "the .bat is Windows only")
    def test_bat_writes_the_rtc_training_iso_beside_itself(self):
        source = shutil.copyfile(self.vanilla, self.tmp / "my vanilla.iso")
        r = self.run_bat(source)
        out = self.folder / "rtc-training.iso"
        self.assertTrue(out.is_file(), r.stdout + r.stderr)
        self.assertEqual(sha1(out), self.iso_sha1)
        self.assertIn("successfully created", r.stdout)
        out.unlink()
        Path(source).unlink()

    @unittest.skipUnless(os.name == "nt", "the .bat is Windows only")
    def test_bat_given_a_modified_iso_says_it_is_not_valid(self):
        r = self.run_bat(self.modified)
        self.assertIn("not a valid v1.02 NTSC melee iso", r.stdout + r.stderr)
        self.assertFalse((self.folder / "rtc-training.iso").exists())

    def run_bat(self, source):
        # As a drag and drop does: cmd /c, so the script pauses at the end
        # (the newline on stdin answers it). Run from elsewhere, so the
        # script must find its files itself.
        return subprocess.run(
            f'cmd /c ""{self.folder / "DRAG VANILLA MELEE HERE.bat"}" "{source}""',
            cwd=self.tmp,
            input="\n",
            capture_output=True,
            text=True,
            timeout=600,
        )

    def test_sh_writes_the_rtc_training_iso_beside_itself(self):
        bash = find_bash()
        if bash is None:
            self.skipTest("no bash")
        env = dict(os.environ)
        if os.name == "nt":
            # Git Bash finds the zip's xdelta3.exe as `xdelta3`.
            env["PATH"] = str(self.folder) + os.pathsep + env["PATH"]
        elif not shutil.which("xdelta3"):
            self.skipTest("no xdelta3 on PATH")
        out = self.folder / "rtc-training.iso"
        r = subprocess.run(
            [str(bash), (self.folder / "build_linux_and_mac.sh").as_posix(), self.vanilla.as_posix()],
            cwd=self.tmp,
            env=env,
            input="\n",
            capture_output=True,
            text=True,
            timeout=600,
        )
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        self.assertEqual(sha1(out), self.iso_sha1)
        out.unlink()
        r = subprocess.run(
            [str(bash), (self.folder / "build_linux_and_mac.sh").as_posix(), self.modified.as_posix()],
            cwd=self.tmp,
            env=env,
            input="\n",
            capture_output=True,
            text=True,
            timeout=600,
        )
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("not a valid v1.02 NTSC melee iso", r.stdout + r.stderr)
        self.assertFalse(out.exists())

    def test_extracted_disc_and_source_disc_are_unchanged(self):
        self.assertEqual(snapshot(self.root / "orig" / "GALE01"), self.orig_before)
        self.assertEqual(sha1(find_disc()), self.disc_before)


if __name__ == "__main__":
    unittest.main()
