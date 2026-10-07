"""Live scenarios (piloting): playing a plan with shot steps, waiting for the
game window, and the CLI's error cases.

No Dolphin is launched and no keys reach a real window: the player is driven
with a fake backend, and the CLI tests use a window-title pattern that cannot
match anything.
"""

import shutil
import sys
import tempfile
import unittest
from pathlib import Path

from .support import REPO_ROOT, dev, make_piloting_sandbox, update_config
from .test_pad_live import DOLPHIN, FakeBackend

sys.path.insert(0, str(REPO_ROOT))
from tools import pad, scenario, scenario_live  # noqa: E402

BINDINGS = {"a": pad.key_from_name("X"), "start": pad.key_from_name("Enter")}


def plan(text):
    folder = Path(tempfile.mkdtemp(prefix="ssbm-scn-"))
    try:
        (folder / "s.txt").write_text(text)
        return scenario.build_plan(scenario.expand("s", folder), BINDINGS)
    finally:
        shutil.rmtree(folder, ignore_errors=True)


class PlayTests(unittest.TestCase):
    def play(self, text, backend=None, shot_fails=False):
        backend = backend or FakeBackend()
        self.shots = []
        self.now = 0.0
        self.printed = []

        def take(label):
            if shot_fails:
                raise pad.PadError("no new screenshot")
            self.shots.append((label, [e[0] for e in backend.log if e[0] in ("down", "up")]))
            return f"/shots/{label}.png"

        scenario_live.play(
            plan(text), DOLPHIN, backend, take, say=self.printed.append, sleep=self.sleep
        )
        return backend

    def sleep(self, s):
        self.now += s

    def test_keys_and_shots_happen_in_order_and_paths_are_printed(self):
        b = self.play("a\nshot one\na\nshot two\n")
        self.assertEqual(
            self.shots,
            [("one", ["down", "up"]), ("two", ["down", "up", "down", "up"])],
        )
        self.assertEqual(self.printed, ["/shots/one.png", "/shots/two.png"])
        self.assertEqual(b.held, set())

    def test_idle_time_before_a_shot_is_waited_out(self):
        self.play("wait:60\nshot one\n")
        self.assertAlmostEqual(self.now, 1.0, places=2)

    def test_pure_waits_and_shot_segments_do_not_take_focus(self):
        b = self.play("wait:60\nshot one\n")
        self.assertEqual([e for e in b.log if e[0] == "focus"], [])

    def test_a_failing_shot_aborts_with_keys_released(self):
        b = FakeBackend()
        with self.assertRaises(pad.PadError):
            self.play("a\nshot one\na\n", b, shot_fails=True)
        self.assertEqual(b.held, set())
        self.assertEqual(sum(1 for e in b.log if e[0] == "down"), 1)

    def test_focus_loss_mid_scenario_aborts_and_releases(self):
        b = FakeBackend(steal_after=1)
        with self.assertRaises(pad.PadError) as cm:
            self.play("a:6\nshot one\n", b)
        self.assertIn("focus lost", str(cm.exception))
        self.assertEqual(b.held, set())
        self.assertEqual(self.shots, [])


class WaitForWindowTests(unittest.TestCase):
    def setUp(self):
        self.now = 0.0

    def sleep(self, s):
        self.now += s

    def test_returns_the_window_once_it_appears(self):
        calls = []

        def find():
            calls.append(1)
            if len(calls) < 3:
                raise pad.PadError("no Dolphin window found")
            return DOLPHIN

        got = scenario_live.wait_for_window(
            find, timeout=30, sleep=self.sleep, clock=lambda: self.now
        )
        self.assertEqual(got, DOLPHIN)

    def test_times_out_with_a_clear_error(self):
        def find():
            raise pad.PadError("no Dolphin window found")

        with self.assertRaises(pad.PadError) as cm:
            scenario_live.wait_for_window(
                find, timeout=30, sleep=self.sleep, clock=lambda: self.now
            )
        self.assertIn("30", str(cm.exception))
        self.assertIn("no Dolphin window found", str(cm.exception))


class FollowMovieTests(unittest.TestCase):
    """Timing the shots of a movie Dolphin is playing, on a fake clock that
    starts when the game window appears (power-on)."""

    def follow(self, text):
        folder = Path(tempfile.mkdtemp(prefix="ssbm-scn-"))
        self.addCleanup(shutil.rmtree, folder, ignore_errors=True)
        (folder / "s.txt").write_text(text)
        self.now = 0.0
        self.shots = []

        def sleep(s):
            self.now += s

        def take(label):
            self.shots.append((label, self.now))
            return f"/shots/{label}.png"

        frames = scenario_live.follow_movie(
            scenario.expand("s", folder), take, say=lambda _m: None,
            sleep=sleep, clock=lambda: self.now,
        )
        return frames

    def test_shots_at_their_frame_and_the_end_after_the_boot_offset(self):
        frames = self.follow("wait:60\nshot one\nwait:30\n")
        self.assertEqual(frames, 90)
        # Measured live: a shot sent N fields (of 1001/60000 s) after the
        # window appears catches frame N to N+4.
        (label, at), = self.shots
        self.assertEqual(label, "one")
        self.assertAlmostEqual(at, 60 * 1001 / 60000, places=3)  # 1.001 s
        # Before Melee's pad setup Dolphin polls once per field, so the last
        # frame is drawn as late as 18 fields after power-on + 90. Returns
        # 1 s after that.
        self.assertAlmostEqual(self.now, 108 * 1001 / 60000 + 1.0, places=2)


class ScenarioLiveCliTests(unittest.TestCase):
    def sandbox(self):
        root, _user = make_piloting_sandbox({"s": "a\nshot one\n"})
        self.addCleanup(shutil.rmtree, root, ignore_errors=True)
        update_config(root / "dev.config.json", dolphin_window_title="^no-such-window-title-xyzzy$")
        return root

    def test_live_scenario_without_a_game_window_sends_nothing(self):
        r = dev(self.sandbox(), "scenario", "s")
        out = r.stdout + r.stderr
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("no Dolphin window", out)

    def test_live_unknown_scenario_is_an_error_before_any_window_lookup(self):
        r = dev(self.sandbox(), "scenario", "nope")
        out = r.stdout + r.stderr
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("unknown scenario", out)

    def test_run_scenario_unknown_name_fails_before_building(self):
        r = dev(self.sandbox(), "run", "--scenario", "nope")
        out = r.stdout + r.stderr
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("unknown scenario", out)
        self.assertNotIn("Launching", out)


if __name__ == "__main__":
    unittest.main()
