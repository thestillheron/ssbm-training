import unittest

from .support import (
    MAIN_DOL_SHA1,
    REPO_ROOT,
    dev,
    find_disc,
    requires_disc,
    sha1,
)


class BuildMatchingTests(unittest.TestCase):
    @requires_disc
    def test_build_matching_produces_byte_identical_main_dol(self):
        r = dev(REPO_ROOT, "setup", str(find_disc()))
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        r = dev(REPO_ROOT, "build", "--matching")
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        # Upstream's default output location.
        dol = REPO_ROOT / "build" / "GALE01" / "main.dol"
        self.assertEqual(sha1(dol), MAIN_DOL_SHA1)

    @requires_disc
    def test_second_build_does_nothing(self):
        r = dev(REPO_ROOT, "build", "--matching")
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        r = dev(REPO_ROOT, "build", "--matching")
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        self.assertNotIn("Configuring", r.stdout)

    @requires_disc
    def test_default_build_is_training_in_its_own_directory(self):
        r = dev(REPO_ROOT, "setup", str(find_disc()))
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        r = dev(REPO_ROOT, "build")
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        self.assertTrue((REPO_ROOT / "build" / "training" / "main.dol").is_file())
        # The matching build is still byte-identical in upstream's location.
        r = dev(REPO_ROOT, "build", "--matching")
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        dol = REPO_ROOT / "build" / "GALE01" / "main.dol"
        self.assertEqual(sha1(dol), MAIN_DOL_SHA1)

    @requires_disc
    def test_switching_builds_does_not_recompile(self):
        for args in (["build"], ["build", "--matching"]):
            r = dev(REPO_ROOT, *args)
            self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        for args in (["build"], ["build", "--matching"], ["build"]):
            r = dev(REPO_ROOT, *args)
            self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
            self.assertNotRegex(r.stdout, r"] (MWCC|AS|LINK|PCH|SPLIT|CTX|DOL) ")

    def test_build_without_setup_explains_what_to_do(self):
        # Runs in a bare sandbox so no disc is needed.
        from .support import make_sandbox
        import shutil

        root = make_sandbox()
        try:
            r = dev(root, "build", "--matching")
            self.assertNotEqual(r.returncode, 0)
            self.assertIn("setup", r.stdout + r.stderr)
        finally:
            shutil.rmtree(root, ignore_errors=True)


if __name__ == "__main__":
    unittest.main()
