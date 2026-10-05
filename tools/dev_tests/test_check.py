"""`dev.py check`: the pre-merge gate, tested in a sandbox copy of the repo.

The sandbox is set up and built once for the whole module, so the real
worktree is never touched and each test only pays for an incremental build.
"""

import re
import shutil
import subprocess
import unittest

from .support import REPO_ROOT, dev, find_disc, requires_disc, make_full_sandbox

# An upstream function with an empty body; the "hook" gives it one without
# the TRAINING_BUILD guard, so it leaks into the matching build.
UPSTREAM_FILE = "src/melee/gm/gmtrainingmode.c"
HOOK_TARGET = "void fn_801B1F6C(int unused) {}"
UNGUARDED_HOOK = "void fn_801B1F6C(int unused) { gm_804D68C0 = 1; }"
TRAINING_SOURCE = "src/training/title_marker.c"


def status_line(output, build):
    """The PASS/FAIL printed for `build` in the summary, or None."""
    m = re.search(rf"^{build}: (PASS|FAIL)\b", output, re.MULTILINE)
    return m.group(1) if m else None


@requires_disc
class CheckTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.root = make_full_sandbox()
        r = dev(cls.root, "setup", str(find_disc()))
        if r.returncode != 0:
            raise RuntimeError(r.stdout + r.stderr)

    @classmethod
    def tearDownClass(cls):
        shutil.rmtree(cls.root, ignore_errors=True)

    def check(self):
        r = dev(self.root, "check")
        return r, r.stdout + r.stderr

    def edit(self, rel, old, new):
        """Edit a sandbox file for the rest of the test; always restored."""
        path = self.root / rel
        original = path.read_text(encoding="utf-8")
        self.assertIn(old, original)
        self.addCleanup(path.write_text, original, encoding="utf-8", newline="")
        path.write_text(original.replace(old, new), encoding="utf-8", newline="")

    def test_clean_tree_passes_both_builds(self):
        r, out = self.check()
        self.assertEqual(r.returncode, 0, out)
        self.assertEqual(status_line(out, "matching"), "PASS", out)
        self.assertEqual(status_line(out, "training"), "PASS", out)

    def test_unguarded_hook_in_upstream_code_fails_matching(self):
        self.edit(UPSTREAM_FILE, HOOK_TARGET, UNGUARDED_HOOK)
        r, out = self.check()
        self.assertNotEqual(r.returncode, 0, out)
        self.assertEqual(status_line(out, "matching"), "FAIL", out)
        # Both builds run even though the first one failed.
        self.assertIn(status_line(out, "training"), ("PASS", "FAIL"), out)

    def test_compile_error_in_training_source_fails_training(self):
        self.edit(TRAINING_SOURCE, "", "\nthis is not valid C;\n")
        r, out = self.check()
        self.assertNotEqual(r.returncode, 0, out)
        self.assertEqual(status_line(out, "training"), "FAIL", out)
        self.assertEqual(status_line(out, "matching"), "PASS", out)

    def test_tree_is_clean_again_after_failures(self):
        r, out = self.check()
        self.assertEqual(r.returncode, 0, out)


if __name__ == "__main__":
    unittest.main()
