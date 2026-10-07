"""`dev.py scenario <name> --movie <path>`: scenario -> Dolphin movie (DTM).

Seam 1 of the scenario-movies spec: the real CLI in a sandbox copy of the repo
with fixture scenarios, reading the written movie back from disk. No Dolphin
is launched, and the sandbox has no pad config, so generating must not need
keyboard bindings.

Expected bytes come from Dolphin's DTM format (Movie.h, little-endian, 256-byte
header then 8 bytes per controller poll) and from decoding a movie the
developer's Dolphin recorded: neutral is `00 40 00 00 80 80 80 80`, Start is
`01 40 ...`, A `02 40 ...`, B `04 40 ...`. Melee polls the pad twice per
emulated frame, so each scenario frame is two identical entries.
"""

import shutil
import struct
import unittest

from .support import REPO_ROOT, dev, make_piloting_sandbox

ENTRIES_PER_FRAME = 2
NEUTRAL = bytes.fromhex("0040000080808080")


class Movie:
    """A written DTM file, decoded only as far as the tests need."""

    def __init__(self, data):
        self.data = data
        self.header = data[:256]
        body = data[256:]
        self.body_len = len(body)
        self.entries = [body[i : i + 8] for i in range(0, len(body), 8)]

    def u64(self, offset):
        return struct.unpack_from("<Q", self.header, offset)[0]

    def frame(self, f):
        """The pad entries for scenario frame f (all should be identical)."""
        return self.entries[f * ENTRIES_PER_FRAME : (f + 1) * ENTRIES_PER_FRAME]

    def frames(self):
        return [self.frame(f) for f in range(len(self.entries) // ENTRIES_PER_FRAME)]


def pad(buttons0=0, buttons1=0, lt=0, rt=0, sx=128, sy=128, cx=128, cy=128):
    return bytes([buttons0, 0x40 | buttons1, lt, rt, sx, sy, cx, cy])


class ScenarioMovieTests(unittest.TestCase):
    def sandbox(self, scenarios):
        # A Dolphin user folder with no GCPadNew.ini: no bindings at all.
        root, _user = make_piloting_sandbox(scenarios, pad_ini=None)
        self.addCleanup(shutil.rmtree, root, ignore_errors=True)
        return root

    def generate(self, scenarios, name="s"):
        root = self.sandbox(scenarios)
        path = root / "out" / "s.dtm"
        r = dev(root, "scenario", name, "--movie", str(path))
        return r, r.stdout + r.stderr, path

    def movie(self, scenarios, name="s"):
        r, out, path = self.generate(scenarios, name)
        self.assertEqual(r.returncode, 0, out)
        return Movie(path.read_bytes()), out, path

    def single(self, step):
        """Pad state of a one-step scenario (one frame)."""
        m, _, _ = self.movie({"s": f"{step}:1\n"})
        self.assertEqual(len(m.frames()), 1)
        entries = m.frame(0)
        self.assertEqual(entries[0], entries[1])
        return entries[0]

    # ------------------------------------------------------------ header

    def test_writes_movie_and_prints_path_and_frame_count(self):
        m, out, path = self.movie({"s": "wait:10\na:5\n"})
        self.assertIn(str(path), out)
        self.assertIn("15 frames", out)

    def test_header_is_power_on_gale01_port1_with_real_memcard(self):
        m, _, _ = self.movie({"s": "wait:10\na:5\n"})
        h = m.header
        self.assertEqual(h[0:4], b"DTM\x1a")
        self.assertEqual(h[4:10], b"GALE01")
        self.assertEqual(h[0x0A], 0)  # not Wii
        self.assertEqual(h[0x0B], 0x01)  # GC port 1 only
        self.assertEqual(h[0x0C], 0)  # power-on, not from a savestate
        self.assertEqual(h[0x89], 1)  # settings come from the header
        self.assertEqual(h[0x97], 0x01)  # memory card in slot A kept
        self.assertEqual(h[0x98], 0)  # existing save used, not cleared
        self.assertEqual(h[0x71:0x81], bytes(16))  # no MD5 check
        self.assertGreaterEqual(m.u64(0xED), 1 << 62)  # tickCount never ends it early

    def test_header_turns_on_fast_disc_speed(self):
        # boot-to-training only fits its time budget with fast disc loads.
        m, _, _ = self.movie({"s": "wait:10\n"})
        self.assertEqual(m.header[0x8E], 1)  # bFastDiscSpeed

    def test_counts_match_the_entries(self):
        m, _, _ = self.movie({"s": "wait:10\na:5\n"})
        self.assertEqual(m.body_len % 8, 0)
        self.assertEqual(len(m.entries), 15 * ENTRIES_PER_FRAME)
        self.assertEqual(m.u64(0x15), len(m.entries))  # inputCount
        self.assertEqual(m.u64(0x0D), 15)  # frameCount
        self.assertEqual(m.u64(0x1D), 0)  # lagCount

    def test_every_entry_is_connected(self):
        m, _, _ = self.movie({"s": "wait:2\nstart+a+b+x+y+z+l+r:2\n"})
        for e in m.entries:
            self.assertTrue(e[1] & 0x40, e.hex())

    # ------------------------------------------------------------ frames

    def test_entry_count_includes_included_scenarios(self):
        m, out, _ = self.movie(
            {"base": "wait:7\nshot one\n", "mid": "include base\nb:4\n",
             "s": "a\ninclude mid\nwait:2\n"}
        )
        # a (default 3) + 7 + 4 + 2
        self.assertEqual(len(m.frames()), 16)
        self.assertIn("16 frames", out)

    def test_holds_land_on_exact_frames_and_the_rest_is_neutral(self):
        m, _, _ = self.movie({"s": "wait:3\na:2\nwait:1\nshot x\nb:3\n"})
        a, b = pad(0x02), pad(0x04)
        expected = [NEUTRAL] * 3 + [a] * 2 + [NEUTRAL] + [b] * 3
        self.assertEqual([f[0] for f in m.frames()], expected)
        for f in m.frames():
            self.assertEqual(f[0], f[1])

    # ------------------------------------------------------------ controls

    def test_buttons_set_their_bits(self):
        expected = {"start": 0x01, "a": 0x02, "b": 0x04, "x": 0x08, "y": 0x10, "z": 0x20}
        for control, bit in expected.items():
            with self.subTest(control=control):
                self.assertEqual(self.single(control), pad(bit))

    def test_triggers_set_digital_bit_and_full_analog(self):
        self.assertEqual(self.single("l"), pad(buttons1=0x04, lt=255))
        self.assertEqual(self.single("r"), pad(buttons1=0x08, rt=255))

    def test_stick_directions_give_full_deflection(self):
        expected = {
            "stick-up": pad(sy=255), "stick-down": pad(sy=0),
            "stick-left": pad(sx=0), "stick-right": pad(sx=255),
            "cstick-up": pad(cy=255), "cstick-down": pad(cy=0),
            "cstick-left": pad(cx=0), "cstick-right": pad(cx=255),
        }
        for control, state in expected.items():
            with self.subTest(control=control):
                self.assertEqual(self.single(control), state)

    def test_dpad_directions_set_their_bits(self):
        expected = {
            "dpad-up": pad(0x40), "dpad-down": pad(0x80),
            "dpad-left": pad(buttons1=0x01), "dpad-right": pad(buttons1=0x02),
        }
        for control, state in expected.items():
            with self.subTest(control=control):
                self.assertEqual(self.single(control), state)

    def test_combined_controls_merge_into_one_state(self):
        self.assertEqual(self.single("stick-up+stick-left"), pad(sx=0, sy=255))
        self.assertEqual(self.single("stick-down+b"), pad(0x04, sy=0))
        self.assertEqual(
            self.single("a+l+cstick-right+dpad-up"),
            pad(0x02 | 0x40, buttons1=0x04, lt=255, cx=255),
        )

    def test_opposing_directions_are_an_error_naming_the_step(self):
        for step in ("stick-up+stick-down", "cstick-left+cstick-right", "dpad-up+dpad-down"):
            with self.subTest(step=step):
                r, out, path = self.generate({"s": f"wait:2\n{step}:4\n"})
                self.assertNotEqual(r.returncode, 0)
                self.assertIn(f"{step}:4", out)
                self.assertIn("s:2", out)
                self.assertFalse(path.exists())

    # ------------------------------------------------------------ errors

    def test_scenario_errors_fail_without_writing(self):
        cases = {
            "bad step": ({"s": "a\nfrobnicate\n"}, "s:2"),
            "unknown control": ({"s": "a\nturbo:3\n"}, "turbo"),
            "missing include": ({"s": "include nope\n"}, "s -> nope"),
            "include cycle": ({"s": "include t\n", "t": "include s\n"}, "s -> t -> s"),
        }
        for label, (scenarios, needle) in cases.items():
            with self.subTest(label):
                r, out, path = self.generate(scenarios)
                self.assertNotEqual(r.returncode, 0)
                self.assertIn(needle, out)
                self.assertFalse(path.exists())

    def test_committed_scenarios_generate(self):
        root = self.sandbox({})
        shutil.rmtree(root / "tools" / "scenarios")
        shutil.copytree(REPO_ROOT / "tools" / "scenarios", root / "tools" / "scenarios",
                        ignore=shutil.ignore_patterns("*.dtm", "*.sav"))
        for name in ("boot-to-character-select", "boot-to-training"):
            with self.subTest(name):
                path = root / f"{name}.dtm"
                r = dev(root, "scenario", name, "--movie", str(path))
                self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
                self.assertGreater(len(Movie(path.read_bytes()).entries), 0)


if __name__ == "__main__":
    unittest.main()
