"""`dev.py pad --dry-run` (piloting): grammar, binding mapping and errors.

Seam 1 of the emulator-piloting spec: the real CLI in a sandbox copy of the
repo, with `dolphin_user` in the sandbox's dev.config.json pointing at a
fixture Dolphin user folder. No Dolphin is launched.
"""

import json
import shutil
import sys
import tempfile
import unittest
from pathlib import Path

from .support import REPO_ROOT, dev

PAD_INI = """\
[GCPad1]
Device = DInput/0/Keyboard Mouse
Buttons/A = X
Buttons/B = Z
Buttons/X = C
Buttons/Start = RETURN
Buttons/Z = `Left Shift`
Main Stick/Up = W
Main Stick/Down = S
Main Stick/Left = A
Main Stick/Right = D
C-Stick/Up = I
C-Stick/Down = K
C-Stick/Left = J
C-Stick/Right = L
D-Pad/Up = T
D-Pad/Down = G
D-Pad/Left = F
D-Pad/Right = H
Triggers/L = COMMA
Triggers/R = PERIOD
[GCPad2]
Device = XInput/0/Gamepad
Buttons/A = `Button A`
"""


def make_sandbox(pad_ini=PAD_INI):
    root = Path(tempfile.mkdtemp(prefix="ssbm-pad-"))
    shutil.copy(REPO_ROOT / "dev.py", root)
    (root / "tools").mkdir()
    for name in ("__init__.py", "pad.py", "pad_live.py", "win_pilot.py"):
        shutil.copy(REPO_ROOT / "tools" / name, root / "tools")
    user = root / "fixture-user"
    (user / "Config").mkdir(parents=True)
    if pad_ini is not None:
        (user / "Config" / "GCPadNew.ini").write_text(pad_ini)
    (root / "dev.config.json").write_text(json.dumps({"dolphin_user": str(user)}))
    return root, user


class PadDryRunTests(unittest.TestCase):
    def setUp(self):
        self.root, self.user = make_sandbox()
        self.addCleanup(shutil.rmtree, self.root, ignore_errors=True)

    def pad(self, seq, *extra):
        r = dev(self.root, "pad", seq, "--dry-run", *extra)
        return r, r.stdout + r.stderr

    def events(self, out):
        """(ms, 'down'|'up', control, scancode) from the printed plan."""
        rows = []
        for line in out.splitlines():
            parts = line.split()
            if len(parts) >= 4 and parts[1] == "ms" and parts[2] in ("down", "up"):
                rows.append((int(parts[0]), parts[2], parts[3], parts[-1]))
        return rows

    def test_single_control_defaults_to_three_frames(self):
        r, out = self.pad("a")
        self.assertEqual(r.returncode, 0, out)
        # 3 frames at 60 fps is 50 ms; A is bound to X (scancode 0x2D).
        self.assertEqual(
            self.events(out), [(0, "down", "a", "0x2D"), (50, "up", "a", "0x2D")]
        )

    def test_combined_controls_with_hold_and_wait(self):
        r, out = self.pad("stick-down+b:6 wait:30 start")
        self.assertEqual(r.returncode, 0, out)
        self.assertEqual(
            self.events(out),
            [
                (0, "down", "stick-down", "0x1F"),
                (0, "down", "b", "0x2C"),
                (100, "up", "stick-down", "0x1F"),
                (100, "up", "b", "0x2C"),
                (600, "down", "start", "0x1C"),
                (650, "up", "start", "0x1C"),
            ],
        )

    def test_every_control_in_the_vocabulary_resolves(self):
        controls = [c for c in "a b x y z start l r".split() if c != "y"]
        for d in ("up", "down", "left", "right"):
            controls += [f"stick-{d}", f"cstick-{d}", f"dpad-{d}"]
        r, out = self.pad(" ".join(controls))
        self.assertEqual(r.returncode, 0, out)
        self.assertEqual(len(self.events(out)), 2 * len(controls))

    def test_special_key_names_map_to_scancodes(self):
        r, out = self.pad("l r z")
        self.assertEqual(r.returncode, 0, out)
        keys = {c: s for _, d, c, s in self.events(out) if d == "down"}
        self.assertEqual(keys, {"l": "0x33", "r": "0x34", "z": "0x2A"})

    def test_malformed_step_is_named(self):
        for bad in ("a:x", "a:", "a++b", "wait", "wait:-1", "a:0"):
            r, out = self.pad(f"a {bad}")
            self.assertNotEqual(r.returncode, 0, bad)
            self.assertIn(bad, out)
            self.assertIn("malformed", out)

    def test_unknown_control(self):
        r, out = self.pad("a+banana")
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("unknown control", out)
        self.assertIn("banana", out)

    def test_unbound_control(self):
        r, out = self.pad("y")
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("unbound", out)
        self.assertIn("y", out)

    def test_key_piloting_cannot_send_is_not_reported_as_unbound(self):
        (self.user / "Config" / "GCPadNew.ini").write_text(
            PAD_INI.replace("Buttons/B = Z", "Buttons/B = WEIRDKEY")
        )
        r, out = self.pad("b")
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("control 'b' is bound to 'WEIRDKEY'", out)
        self.assertIn("can't send", out)
        self.assertNotIn("unbound", out)

    def test_numpad_and_caps_lock_bindings_map_to_scancodes(self):
        (self.user / "Config" / "GCPadNew.ini").write_text(
            PAD_INI.replace("Buttons/B = Z", "Buttons/B = NUMPAD1").replace(
                "Buttons/X = C", "Buttons/X = CAPITAL"
            )
        )
        r, out = self.pad("b x")
        self.assertEqual(r.returncode, 0, out)
        keys = {c: s for _, d, c, s in self.events(out) if d == "down"}
        self.assertEqual(keys, {"b": "0x4F", "x": "0x3A"})

    def test_port_one_device_must_be_the_keyboard(self):
        (self.user / "Config" / "GCPadNew.ini").write_text(
            PAD_INI.replace("DInput/0/Keyboard Mouse", "XInput/0/Gamepad")
        )
        r, out = self.pad("a")
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("keyboard", out.lower())
        self.assertIn("XInput/0/Gamepad", out)

    def test_missing_pad_config(self):
        (self.user / "Config" / "GCPadNew.ini").unlink()
        r, out = self.pad("a")
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("GCPadNew.ini", out)

    def test_bindings_are_read_fresh_each_time(self):
        self.assertEqual(self.events(self.pad("a")[1])[0][3], "0x2D")
        (self.user / "Config" / "GCPadNew.ini").write_text(
            PAD_INI.replace("Buttons/A = X", "Buttons/A = V")
        )
        self.assertEqual(self.events(self.pad("a")[1])[0][3], "0x2F")

    @unittest.skipUnless(sys.platform == "darwin", "macOS only")
    def test_macos_is_not_supported_yet(self):
        r, out = self.pad("a")
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("not supported yet", out)


if __name__ == "__main__":
    unittest.main()
