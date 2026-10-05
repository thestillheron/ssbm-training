"""Live `pad` (piloting): focus guard, release-on-failure, zero-window error,
and PID tracking of the Dolphin that `run` launches.

No Dolphin is launched and no keys reach a real window: the sender is driven
with a fake backend, and the CLI tests use a window-title pattern that cannot
match anything.
"""

import json
import shutil
import sys
import unittest

from .support import REPO_ROOT, dev
from .test_pad import make_sandbox

sys.path.insert(0, str(REPO_ROOT))
from tools import pad, pad_live  # noqa: E402

DOLPHIN = 100
OTHER = 200
EDITOR = 300


class FakeBackend:
    """Records what would be sent; `steal_after` hands focus to another
    window after that many key events."""

    def __init__(self, steal_after=None, explode_after=None, fail_release=()):
        self.fail_release = set(fail_release)
        self.fg = EDITOR
        self.log = []
        self.held = set()
        self.sent = 0
        self.steal_after = steal_after
        self.explode_after = explode_after

    def foreground(self):
        return self.fg

    def focus(self, hwnd):
        self.log.append(("focus", hwnd))
        self.fg = hwnd

    def key(self, key, down):
        if self.explode_after is not None and self.sent >= self.explode_after:
            self.explode_after = None  # interrupt once, releases must work
            raise KeyboardInterrupt
        if not down and key.name in self.fail_release:
            raise OSError(f"cannot release {key.name}")
        self.sent += 1
        self.log.append(("down" if down else "up", key.name))
        (self.held.add if down else self.held.discard)(key.name)
        if self.steal_after is not None and self.sent >= self.steal_after:
            self.fg = OTHER


def plan(text="a+b:3"):
    bindings = {
        "a": pad.key_from_name("X"),
        "b": pad.key_from_name("Z"),
    }
    return pad.build_plan(pad.parse_sequence(text), bindings)


def run(events, backend):
    pad_live.run_plan(
        events, DOLPHIN, backend, sleep=lambda s: None, clock=lambda: 0.0
    )


class RunPlanTests(unittest.TestCase):
    def test_sends_all_events_then_restores_previous_focus(self):
        b = FakeBackend()
        run(plan(), b)
        self.assertEqual(b.sent, 4)
        self.assertEqual(b.held, set())
        self.assertEqual(b.log[0], ("focus", DOLPHIN))
        self.assertEqual(b.log[-1], ("focus", EDITOR))

    def test_focus_lost_releases_held_keys_and_fails(self):
        b = FakeBackend(steal_after=2)  # both downs sent, then focus is stolen
        with self.assertRaisesRegex(pad.PadError, "focus lost"):
            run(plan(), b)
        self.assertEqual(b.held, set())
        self.assertEqual(b.sent, 4)  # 2 downs + 2 releases, no further plan events

    def test_interrupt_releases_held_keys_and_restores_focus(self):
        b = FakeBackend(explode_after=2)
        with self.assertRaises(KeyboardInterrupt):
            run(plan(), b)
        self.assertEqual(b.held, set())
        self.assertEqual(b.log[-1], ("focus", EDITOR))

    def test_does_not_refocus_if_dolphin_was_already_in_front(self):
        b = FakeBackend()
        b.fg = DOLPHIN
        run(plan(), b)
        self.assertNotIn(("focus", EDITOR), b.log)


    def test_one_failing_release_does_not_stop_the_others(self):
        b = FakeBackend(steal_after=2, fail_release={"X"})
        with self.assertRaises(pad.PadError) as cm:
            run(plan(), b)
        self.assertIn("X", str(cm.exception))
        self.assertEqual(b.held, {"X"})  # Z was still released

    def test_no_restore_focus_call_when_nothing_was_in_front(self):
        b = FakeBackend()
        b.fg = 0
        run(plan(), b)
        self.assertEqual(b.log.count(("focus", 0)), 0)

    def test_focus_is_not_taken_before_the_first_event_is_due(self):
        b = FakeBackend()
        seen = []
        pad_live.run_plan(
            plan("wait:60 a"),
            DOLPHIN,
            b,
            sleep=lambda s: seen.append(list(b.log)),
            clock=lambda: 0.0,
        )
        self.assertEqual(seen[0], [])  # nothing focused while waiting

    def test_window_switch_during_an_idle_wait_is_not_an_abort(self):
        b = FakeBackend()

        def sleep(_s):
            if not b.held:
                b.fg = OTHER  # user switches windows during an idle wait

        pad_live.run_plan(
            plan("a wait:60 a"), DOLPHIN, b, sleep=sleep, clock=lambda: 0.0
        )
        self.assertEqual(b.held, set())
        self.assertEqual(b.sent, 4)


class LiveCliTests(unittest.TestCase):
    def setUp(self):
        self.root, self.user = make_sandbox()
        self.addCleanup(shutil.rmtree, self.root, ignore_errors=True)
        cfg = self.root / "dev.config.json"
        data = json.loads(cfg.read_text())
        # A title that no window can have, so a real Dolphin on this machine
        # is never found (and never receives keys) by these tests.
        data["dolphin_window_title"] = "^no-such-window-title-9f3a$"
        cfg.write_text(json.dumps(data))

    @unittest.skipUnless(sys.platform == "win32", "Windows only")
    def test_no_dolphin_window_is_a_clear_error_and_sends_nothing(self):
        r = dev(self.root, "pad", "a")
        out = r.stdout + r.stderr
        self.assertNotEqual(r.returncode, 0, out)
        self.assertIn("no Dolphin window", out)
        self.assertIn("dolphin_window_title", out)


class TrackedDolphinTests(unittest.TestCase):
    def setUp(self):
        import dev as devmod

        self.dev = devmod
        self.killed = []
        self.images = {}

    def stop(self, state):
        return self.dev.stop_tracked_dolphin(
            state,
            image_of=self.images.get,
            terminate=self.killed.append,
        )

    def test_terminates_recorded_pid_when_still_dolphin(self):
        self.images[4242] = r"C:\Tools\Dolphin\Dolphin.exe"
        self.stop({"dolphin_pid": 4242})
        self.assertEqual(self.killed, [4242])

    def test_pid_reused_by_another_program_is_left_alone(self):
        self.images[4242] = r"C:\Windows\notepad.exe"
        self.stop({"dolphin_pid": 4242})
        self.assertEqual(self.killed, [])

    def test_only_the_exact_dolphin_image_name_counts(self):
        self.images[1] = r"C:\Tools\DolphinTool\launcher.exe"
        self.images[2] = r"C:\Tools\DolphinWatcher.exe"
        self.images[3] = r"C:\Tools\DOLPHIN.EXE"
        for pid in (1, 2, 3):
            self.stop({"dolphin_pid": pid})
        self.assertEqual(self.killed, [3])

    def test_a_failed_terminate_is_surfaced(self):
        self.images[4242] = r"C:\Tools\Dolphin\Dolphin.exe"

        def refuse(pid):
            raise OSError("access denied")

        with self.assertRaisesRegex(self.dev.DevError, "4242.*access denied"):
            self.dev.stop_tracked_dolphin(
                {"dolphin_pid": 4242}, image_of=self.images.get, terminate=refuse
            )

    def test_dead_or_missing_pid_is_ignored(self):
        self.stop({"dolphin_pid": 4242})
        self.stop({})
        self.assertEqual(self.killed, [])


if __name__ == "__main__":
    unittest.main()
