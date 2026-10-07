"""`dev.py reps`: the rep log lines out of Dolphin's log file.

Driven through the real command line in a sandbox with a fixture Dolphin user
folder and fixture log files; no Dolphin is launched.
"""

import json
import shutil
import subprocess
import sys
import unittest

from .support import dev, make_piloting_sandbox

LOG = (
    "00:00:01:000 Boot (I): starting\n"
    "00:00:02:000 OSREPORT (I): [rep] event=start rep=1 frame=100\n"
    "00:00:02:100 OSREPORT (I): something else entirely\n"
    "00:00:03:000 OSREPORT (I): [rep] event=landing rep=1 frame=140\n"
)

SETTINGS_ON = "[Options]\nWriteToFile = True\n[Logs]\nOSREPORT = True\n"


class RepsCliTests(unittest.TestCase):
    def sandbox(self, log=LOG, settings=SETTINGS_ON):
        root, user = make_piloting_sandbox()
        self.addCleanup(shutil.rmtree, root, ignore_errors=True)
        if log is not None:
            (user / "Logs").mkdir()
            (user / "Logs" / "dolphin.log").write_text(log)
        if settings is not None:
            (user / "Config" / "Logger.ini").write_text(settings)
        return root, user

    def test_prints_only_rep_prefixed_lines(self):
        root, _ = self.sandbox()
        r = dev(root, "reps")
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        self.assertEqual(
            r.stdout.splitlines(),
            ["[rep] event=start rep=1 frame=100", "[rep] event=landing rep=1 frame=140"],
        )

    def test_prints_only_lines_after_the_start_marker(self):
        root, user = self.sandbox()
        log = user / "Logs" / "dolphin.log"
        mark = len(log.read_bytes())
        with open(log, "a") as f:
            f.write("t OSREPORT (I): [rep] event=start rep=2 frame=900\n")
        (root / "build").mkdir()
        (root / "build" / "dev_state.json").write_text(json.dumps({"rep_log_mark": mark}))
        r = dev(root, "reps")
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        self.assertEqual(r.stdout.splitlines(), ["[rep] event=start rep=2 frame=900"])

    def test_missing_log_says_how_to_get_one(self):
        root, _ = self.sandbox(log=None, settings=None)
        r = dev(root, "reps")
        self.assertNotEqual(r.returncode, 0)
        for needle in ("dev.py run", "Write to File", "OSREPORT", "Logger.ini", "WriteToFile"):
            self.assertIn(needle, r.stderr)

    def test_logger_ini_off_is_fine_because_run_overrides_it_per_launch(self):
        root, _ = self.sandbox(settings="[Options]\nWriteToFile = False\n[Logs]\nOSREPORT = False\n")
        r = dev(root, "reps")
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        self.assertEqual(len(r.stdout.splitlines()), 2)

    def test_log_lines_as_dolphin_writes_them(self):
        # Seen in a real Dolphin log: no hour column, and the line ends
        # "\r\r\n" (the game's own "\n" plus Dolphin's).
        real = (
            b"01:48:643 Core/HW/EXI/EXI_DeviceIPL.cpp:306 N[OSREPORT]: \r\r\n"
            b"03:57:455 Core/HW/EXI/EXI_DeviceIPL.cpp:306 N[OSREPORT]: [rep] event=probe\r\r\n"
        )
        root, _ = self.sandbox(log=None)
        fixture = root / "real.log"
        fixture.write_bytes(real)
        r = dev(root, "reps", "--log", str(fixture))
        self.assertEqual(r.stdout.splitlines(), ["[rep] event=probe"])

    def test_dry_run_is_not_an_option_because_reps_only_ever_reads(self):
        root, _ = self.sandbox()
        r = dev(root, "reps", "--dry-run")
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("--dry-run", r.stderr)

    def test_log_option_reads_the_whole_file_not_from_the_runs_marker(self):
        root, _ = self.sandbox(log=None, settings=None)
        fixture = root / "fixture.log"
        fixture.write_text(LOG)
        (root / "build").mkdir()
        (root / "build" / "dev_state.json").write_text(json.dumps({"rep_log_mark": len(LOG)}))
        r = dev(root, "reps", "--log", str(fixture))
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        self.assertEqual(len(r.stdout.splitlines()), 2)

    def test_a_run_without_authoring_tools_is_reported_as_not_logging(self):
        root, _ = self.sandbox()
        (root / "build").mkdir()
        (root / "build" / "dev_state.json").write_text(json.dumps({"rep_log_mark": None}))
        r = dev(root, "reps")
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("--no-authoring", r.stderr)
        self.assertNotIn("no [rep] lines", r.stderr)

    def test_the_empty_note_says_which_start_marker_was_used(self):
        root, _ = self.sandbox(log="t Boot (I): hi\n")
        r = dev(root, "reps", "--since", "3")
        self.assertIn("after byte 3", r.stderr)
        self.assertNotIn("last `run`", r.stderr)
        (root / "build").mkdir()
        (root / "build" / "dev_state.json").write_text(json.dumps({"rep_log_mark": 0}))
        r = dev(root, "reps")
        self.assertIn("last `run`", r.stderr)

    def test_no_reps_in_a_working_log_is_not_an_error(self):
        root, _ = self.sandbox(log="t Boot (I): hi\n")
        r = dev(root, "reps")
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        self.assertEqual(r.stdout.strip(), "")
        self.assertIn("no [rep] lines", r.stderr)


class ScenarioRepLogTests(unittest.TestCase):
    """`dev.py scenario` prints the rep lines Dolphin logs while it plays. A
    scenario with no key or shot steps needs no game window, so the command
    runs without Dolphin; the test plays Dolphin by appending to the log
    after the command says it started."""

    def run_scenario(self, before, during, log_exists=True):
        root, user = make_piloting_sandbox({"quiet": "wait:10\n"})
        self.addCleanup(shutil.rmtree, root, ignore_errors=True)
        if log_exists:
            (user / "Logs").mkdir()
            (user / "Logs" / "dolphin.log").write_text(before)
        proc = subprocess.Popen(
            [sys.executable, str(root / "dev.py"), "scenario", "quiet"],
            cwd=root,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        self.addCleanup(proc.kill)
        self.assertIn("quiet", proc.stdout.readline())  # started; its marker is taken
        if log_exists:
            with open(user / "Logs" / "dolphin.log", "a") as f:
                f.write(during)
        out, err = proc.communicate(timeout=60)
        return proc.returncode, out, err

    def test_prints_only_the_rep_lines_written_since_the_scenario_started(self):
        code, out, _ = self.run_scenario(
            LOG,
            "00:00:09:000 OSREPORT (I): [rep] event=drop rep=2 frame=61 height=40\n"
            "00:00:09:500 OSREPORT (I): not a rep line\n"
            "00:00:10:000 OSREPORT (I): [rep] event=landing rep=2 frame=99\n",
        )
        self.assertEqual(code, 0)
        self.assertEqual(
            out.splitlines(),
            ["[rep] event=drop rep=2 frame=61 height=40", "[rep] event=landing rep=2 frame=99"],
        )

    def test_a_scenario_with_no_reps_says_so_on_stderr_and_succeeds(self):
        code, out, err = self.run_scenario(LOG, "")
        self.assertEqual(code, 0)
        self.assertEqual(out, "")
        self.assertIn("no [rep] lines", err)

    def test_missing_log_is_reported_not_mistaken_for_no_reps(self):
        code, out, err = self.run_scenario("", "", log_exists=False)
        self.assertEqual(code, 0)
        self.assertIn("has not written its game log", err)


if __name__ == "__main__":
    unittest.main()
