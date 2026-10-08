"""The authoring flag: `build`/`run` `--authoring` / `--no-authoring`, the
recorded variant, and `check` reporting every variant.

Builds happen in a sandbox copy of the repo, set up and built once for the
module.
"""

import json
import re
import shutil
import unittest

from .support import (
    dev,
    find_disc,
    make_full_sandbox,
    make_sandbox,
    requires_disc,
)

# Compiled in by the authoring macro only (src/training/authoring.c).
MARKER = b"AUTHORING_BUILD_MARKER"
REP_PREFIX = b"[rep]"  # the rep log prefix, authoring-only
REP_EVENTS = (b"start", b"drop", b"landing", b"tech", b"actionable", b"outcome")
COMPILE_LINE = re.compile(r"\] MWCC (\S+)")


def state(root):
    return json.loads((root / "build" / "dev_state.json").read_text())


def compiled(output):
    """Object files compiled, as forward-slash paths (ninja prints the
    platform's separator)."""
    return [f.replace("\\", "/") for f in COMPILE_LINE.findall(output)]


class FlagParsingTests(unittest.TestCase):
    """No disc needed: a bare sandbox fails at the missing setup, which is
    after the arguments were accepted."""

    def setUp(self):
        self.root = make_sandbox()
        self.addCleanup(shutil.rmtree, self.root, ignore_errors=True)

    def accepted(self, *args):
        r = dev(self.root, *args)
        self.assertIn("setup", r.stdout + r.stderr)
        self.assertNotIn("usage:", r.stdout + r.stderr)

    def test_build_accepts_both_flags(self):
        self.accepted("build", "--authoring")
        self.accepted("build", "--no-authoring")

    def test_later_flag_wins(self):
        self.accepted("build", "--authoring", "--no-authoring")

    def test_matching_build_has_no_authoring_variant(self):
        r = dev(self.root, "build", "--matching", "--authoring")
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("authoring", r.stdout + r.stderr)


@requires_disc
class AuthoringBuildTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.root = make_full_sandbox()
        r = dev(cls.root, "setup", str(find_disc()))
        if r.returncode != 0:
            raise RuntimeError(r.stdout + r.stderr)

    @classmethod
    def tearDownClass(cls):
        shutil.rmtree(cls.root, ignore_errors=True)

    def build(self, *args):
        r = dev(self.root, "build", *args)
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        return r.stdout

    def dol(self):
        return (self.root / "build" / "training" / "main.dol").read_bytes()

    def test_default_build_has_no_authoring_tools(self):
        self.build()
        self.assertNotIn(MARKER, self.dol())
        self.assertNotIn(REP_PREFIX, self.dol())
        self.assertFalse(state(self.root)["authoring"])

    def test_authoring_build_includes_them_and_records_the_variant(self):
        self.build("--authoring")
        self.assertIn(MARKER, self.dol())
        for event in REP_EVENTS:
            self.assertIn(REP_PREFIX + b" event=" + event, self.dol())
        self.assertNotIn(b"event=probe", self.dol())
        for field in (b"window_open=", b"vuln_from=", b"vuln_to=", b"actionable="):
            self.assertIn(field, self.dol())
        self.assertTrue(state(self.root)["authoring"])

    def test_switching_variants_recompiles_only_training_code(self):
        self.build("--authoring")
        for args in ([], ["--authoring"], []):
            out = self.build(*args)
            files = compiled(out)
            self.assertTrue(files, out)
            self.assertTrue(all("/src/training/" in f for f in files), files)

    def test_unchanged_variant_does_not_reconfigure(self):
        self.build("--authoring")
        self.assertNotIn("Configuring", self.build("--authoring"))
        self.assertIn("Configuring", self.build())

    def test_check_reports_every_variant(self):
        r = dev(self.root, "check")
        out = r.stdout + r.stderr
        self.assertEqual(r.returncode, 0, out)
        for name in ("matching", "training", "training-authoring"):
            self.assertRegex(out, rf"(?m)^{name}: PASS\b")

    def test_authoring_compile_error_fails_check(self):
        path = self.root / "src" / "training" / "authoring.c"
        original = path.read_text(encoding="utf-8")
        self.addCleanup(path.write_text, original, encoding="utf-8", newline="")
        path.write_text(
            original.replace(
                "#ifdef AUTHORING_BUILD", "#ifdef AUTHORING_BUILD\nnot valid C;", 1
            ),
            encoding="utf-8",
            newline="",
        )
        r = dev(self.root, "check")
        out = r.stdout + r.stderr
        self.assertNotEqual(r.returncode, 0, out)
        self.assertRegex(out, r"(?m)^training-authoring: FAIL\b")
        self.assertRegex(out, r"(?m)^training: PASS\b")
        self.assertRegex(out, r"(?m)^matching: PASS\b")


if __name__ == "__main__":
    unittest.main()
