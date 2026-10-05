"""Opt-in end-to-end test (Seam 2): `dev.py run --scenario` against a real
Dolphin.

It takes window focus and sends keystrokes, so it is skipped unless
SSBM_E2E=1 is set, and also skipped without a disc image or a Dolphin. It is
not part of `dev.py check`. It runs in the real repo checkout and uses (and
finally terminates) the Dolphin that `run` launches there.
"""

import json
import os
import re
import sys
import time
import unittest
from pathlib import Path

from .support import REPO_ROOT, dev, find_disc

sys.path.insert(0, str(REPO_ROOT))

ENABLED = os.environ.get("SSBM_E2E") == "1"


def _dolphin_found():
    try:
        import dev as dev_module

        return dev_module.find_dolphin() is not None
    except Exception:
        return False


STATE = REPO_ROOT / "build" / "dev_state.json"


def tracked_pid():
    return json.loads(STATE.read_text()).get("dolphin_pid")


def alive(pid):
    from tools import win_pilot

    image = win_pilot.process_image(pid)
    return bool(image) and "dolphin" in Path(image).stem.lower()


@unittest.skipUnless(ENABLED, "set SSBM_E2E=1 to run (takes focus, boots Dolphin)")
@unittest.skipUnless(sys.platform == "win32", "piloting is Windows only")
@unittest.skipUnless(find_disc(), "no disc image: set SSBM_TEST_DISC or put one in orig/GALE01/")
@unittest.skipUnless(_dolphin_found() if ENABLED else False, "no Dolphin found")
class LiveScenarioEndToEnd(unittest.TestCase):
    def shot_path(self, out, label):
        for line in out.splitlines():
            if line.strip().endswith(f"{label}.png"):
                return Path(line.strip())
        self.fail(f"no path for shot {label!r} printed:\n{out}")

    def test_run_scenario_takes_a_shot_and_the_next_run_replaces_the_instance(self):
        from tools import win_pilot

        self.addCleanup(self.stop_launched)
        r = dev(REPO_ROOT, "run", "--scenario", "e2e-title")
        out = r.stdout + r.stderr
        self.assertEqual(r.returncode, 0, out)
        png = self.shot_path(out, "title")
        self.assertTrue(png.is_file(), png)
        self.assertGreater(png.stat().st_size, 0)
        first = tracked_pid()
        self.assertTrue(alive(first))

        r = dev(REPO_ROOT, "run", "--scenario", "e2e-smoke")
        out = r.stdout + r.stderr
        self.assertEqual(r.returncode, 0, out)
        self.assertTrue(self.shot_path(out, "e2e-smoke").is_file())
        second = tracked_pid()
        self.assertNotEqual(first, second)
        for _ in range(50):
            if not alive(first):
                break
            time.sleep(0.1)
        self.assertFalse(alive(first), "the previous instance should have been replaced")
        self.assertTrue(alive(second))

    def stop_launched(self):
        from tools import win_pilot

        pid = tracked_pid() if STATE.exists() else None
        if pid and alive(pid):
            win_pilot.terminate(pid)
