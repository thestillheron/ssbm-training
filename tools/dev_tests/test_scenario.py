"""`dev.py scenario --dry-run`: scenario files, includes, shots and errors.

Seam 1 of the emulator-piloting spec: the real CLI in a sandbox copy of the
repo with fixture scenarios in the sandbox's tools/scenarios and a fixture pad
config via `dolphin_user`. No Dolphin is launched.
"""

import shutil
import sys
import unittest

from .support import REPO_ROOT, dev, make_piloting_sandbox


class ScenarioDryRunTests(unittest.TestCase):
    def sandbox(self, scenarios):
        root, _user = make_piloting_sandbox(scenarios)
        self.addCleanup(shutil.rmtree, root, ignore_errors=True)
        return root

    def run_scenario(self, root, name):
        r = dev(root, "scenario", name, "--dry-run")
        return r, r.stdout + r.stderr

    def rows(self, out):
        """(ms, kind, detail) from the plan: kind is down|up|shot."""
        rows = []
        for line in out.splitlines():
            p = line.split()
            if len(p) >= 4 and p[1] == "ms" and p[2] in ("down", "up", "shot"):
                rows.append((int(p[0]), p[2], p[3]))
        return rows

    def test_comments_and_blank_lines_are_ignored(self):
        root = self.sandbox(
            {"s": "# a comment\n\na\n  # indented comment\nwait:30\nstart:6\n"}
        )
        r, out = self.run_scenario(root, "s")
        self.assertEqual(r.returncode, 0, out)
        # a: 3 frames (50 ms), wait 30 frames (500 ms), start 6 frames (100 ms)
        self.assertEqual(
            self.rows(out),
            [(0, "down", "a"), (50, "up", "a"),
             (550, "down", "start"), (650, "up", "start")],
        )

    def test_shot_appears_at_its_position(self):
        root = self.sandbox({"s": "a\nshot title\nb\n"})
        r, out = self.run_scenario(root, "s")
        self.assertEqual(r.returncode, 0, out)
        self.assertEqual(
            self.rows(out),
            [(0, "down", "a"), (50, "up", "a"), (50, "shot", "title"),
             (50, "down", "b"), (100, "up", "b")],
        )

    def test_include_is_expanded_in_place(self):
        root = self.sandbox({"base": "a\nshot one\n", "s": "b\ninclude base\nb\n"})
        r, out = self.run_scenario(root, "s")
        self.assertEqual(r.returncode, 0, out)
        self.assertEqual(
            [(k, d) for _, k, d in self.rows(out)],
            [("down", "b"), ("up", "b"), ("down", "a"), ("up", "a"),
             ("shot", "one"), ("down", "b"), ("up", "b")],
        )

    def test_include_cycle_names_the_chain(self):
        root = self.sandbox({"a": "include b\n", "b": "include c\n", "c": "include a\n"})
        r, out = self.run_scenario(root, "a")
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("a -> b -> c -> a", out)

    def test_missing_include_names_the_chain(self):
        root = self.sandbox({"a": "include b\n", "b": "include nope\n"})
        r, out = self.run_scenario(root, "a")
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("a -> b -> nope", out)

    def test_unknown_scenario_lists_available(self):
        root = self.sandbox({"alpha": "a\n", "beta": "b\n"})
        r, out = self.run_scenario(root, "gamma")
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("gamma", out)
        self.assertIn("alpha", out)
        self.assertIn("beta", out)

    def test_bad_step_names_scenario_and_line(self):
        root = self.sandbox({"s": "a\nfrobnicate\n"})
        r, out = self.run_scenario(root, "s")
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("s:2", out)
        self.assertIn("frobnicate", out)

    def test_shot_without_label_is_an_error(self):
        root = self.sandbox({"s": "shot\n"})
        r, out = self.run_scenario(root, "s")
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("s:1", out)

    def test_committed_boot_to_character_select_passes_dry_run(self):
        root, _ = make_piloting_sandbox()
        self.addCleanup(shutil.rmtree, root, ignore_errors=True)
        shutil.copytree(REPO_ROOT / "tools" / "scenarios", root / "tools" / "scenarios")
        r, out = self.run_scenario(root, "boot-to-character-select")
        self.assertEqual(r.returncode, 0, out)
        self.assertTrue(self.rows(out))

    @unittest.skipUnless(sys.platform == "darwin", "macOS only")
    def test_macos_is_not_supported_yet(self):
        root = self.sandbox({"s": "a\n"})
        r, out = self.run_scenario(root, "s")
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("not supported yet", out)


if __name__ == "__main__":
    unittest.main()
