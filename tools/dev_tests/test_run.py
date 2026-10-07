import shutil
import unittest
from pathlib import Path

from .support import (
    REPO_ROOT,
    FakeDolphinCase,
    RunScenarioCase,
    dev,
    find_disc,
    fixture_card,
    launched_args,
    make_fake_dolphin,
    make_run_sandbox,
    requires_disc,
    sha1,
    write_dolphin_ini,
)

ORIG = REPO_ROOT / "orig" / "GALE01"


def original_fingerprint():
    return {
        p.relative_to(ORIG).as_posix(): sha1(p)
        for sub in ("sys", "files")
        for p in sorted((ORIG / sub).rglob("*"))
        if p.is_file() and p.name != ".gitkeep"
    }


class RunTests(FakeDolphinCase):
    def test_run_without_dolphin_explains_how_to_set_it(self):
        self.configure_dolphin(Path(self.tmp.name) / "no-such-dolphin")
        r = dev(REPO_ROOT, "run")
        out = r.stdout + r.stderr
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("dev.config.json", out)
        self.assertIn("dolphin", out.lower())

    @requires_disc
    def test_run_assembles_training_game_and_launches_dolphin(self):
        r = dev(REPO_ROOT, "setup", str(find_disc()))
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        before = original_fingerprint()
        exe, args_file = make_fake_dolphin(self.tmp.name)
        self.configure_dolphin(exe)

        r = dev(REPO_ROOT, "run")
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)

        game = REPO_ROOT / "build" / "training" / "game"
        game_dol = game / "sys" / "main.dol"
        training_dol = REPO_ROOT / "build" / "training" / "main.dol"
        self.assertTrue(game_dol.is_file())
        self.assertEqual(sha1(game_dol), sha1(training_dol))
        # Dolphin got the assembled game's main.dol.
        launched = launched_args(self, args_file)
        self.assertTrue(any(Path(a) == game_dol for a in launched), f"args were {launched}")
        # The rest of the disc is mirrored into the game folder.
        self.assertTrue((game / "sys" / "boot.bin").is_file())
        self.assertEqual(
            len([p for p in (game / "files").rglob("*") if p.name != ".gitkeep"]),
            len([p for p in (ORIG / "files").rglob("*") if p.name != ".gitkeep"]),
        )
        # The extracted original files were never written to.
        self.assertEqual(original_fingerprint(), before)

        # Running again works on an existing game folder and still leaves the
        # originals alone.
        r = dev(REPO_ROOT, "run")
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        self.assertEqual(original_fingerprint(), before)

    @requires_disc
    def test_run_clears_the_shots_folder(self):
        r = dev(REPO_ROOT, "setup", str(find_disc()))
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        exe, _args = make_fake_dolphin(self.tmp.name)
        self.configure_dolphin(exe)
        shots = REPO_ROOT / "build" / "training" / "shots"
        shots.mkdir(parents=True, exist_ok=True)
        (shots / "stale.png").write_bytes(b"x")

        r = dev(REPO_ROOT, "run")
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        self.assertFalse((shots / "stale.png").exists())


class RunScenarioLaunchTests(RunScenarioCase):
    """`run --scenario` up to the launch, with the real checkout's build and
    its e2e-smoke scenario."""

    def arg_after(self, args, flag):
        self.assertIn(flag, args, f"args were {args}")
        return Path(args[args.index(flag) + 1])

    def test_boots_the_training_build_with_the_scenarios_movie(self):
        r = dev(REPO_ROOT, "run", "--scenario", "e2e-smoke")
        out = r.stdout + r.stderr
        args = self.launched_args()
        game_dol = REPO_ROOT / "build" / "training" / "game" / "sys" / "main.dol"
        self.assertEqual(self.arg_after(args, "-e"), game_dol)
        movie = self.arg_after(args, "-m")
        self.assertTrue(movie.is_relative_to(REPO_ROOT / "build"), movie)
        # The movie is the one `scenario --movie` generates.
        expected = Path(self.tmp.name) / "expected.dtm"
        r2 = dev(REPO_ROOT, "scenario", "e2e-smoke", "--movie", str(expected))
        self.assertEqual(r2.returncode, 0, r2.stdout + r2.stderr)
        self.assertEqual(movie.read_bytes(), expected.read_bytes())
        # No game window from the fake: a clear error, not a silent success.
        self.assertNotEqual(r.returncode, 0, out)
        self.assertIn("did not appear", out)

    def test_live_boots_without_a_movie(self):
        r = dev(REPO_ROOT, "run", "--scenario", "e2e-smoke", "--live")
        args = self.launched_args()
        self.assertNotIn("-m", args)
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("did not appear", r.stdout + r.stderr)


class RunScenarioMovieChecksTests(unittest.TestCase):
    """`run --scenario` checks that need no build: they fail before anything
    is built or launched. A sandbox copy of the repo with a fixture scenario
    and a fake Dolphin."""

    def sandbox(self, movie_option=True, retro_achievements=None):
        root, self.user, self.args_file = make_run_sandbox(
            {"s": "wait:60\nshot one\n"}, movie_option
        )
        self.addCleanup(shutil.rmtree, root, ignore_errors=True)
        write_dolphin_ini(self.user, card=fixture_card(self.user))
        if retro_achievements is not None:
            (self.user / "Config" / "RetroAchievements.ini").write_text(retro_achievements)
        return root

    def assert_failed_before_building(self, r):
        out = r.stdout + r.stderr
        self.assertNotEqual(r.returncode, 0, out)
        self.assertNotIn("Building", out)
        self.assertNotIn("Launching", out)
        self.assertFalse(self.args_file.exists(), "Dolphin should not have been launched")

    def test_a_dolphin_without_movie_playback_is_a_clear_error_not_a_fallback(self):
        r = dev(self.sandbox(movie_option=False), "run", "--scenario", "s")
        self.assert_failed_before_building(r)
        out = r.stdout + r.stderr
        self.assertIn("movie", out.lower())
        self.assertIn("--live", out)

    def test_retroachievements_hardcore_mode_is_a_clear_error(self):
        # Dolphin silently refuses to play movies in hardcore mode.
        ini = "[Achievements]\nEnabled = True\nHardcoreEnabled = True\n"
        r = dev(self.sandbox(retro_achievements=ini), "run", "--scenario", "s")
        self.assert_failed_before_building(r)
        self.assertIn("hardcore", (r.stdout + r.stderr).lower())

    def test_movie_playback_needs_no_pad_bindings(self):
        root = self.sandbox()
        (self.user / "Config" / "GCPadNew.ini").unlink()
        r = dev(root, "run", "--scenario", "s")
        # Gets as far as building (the sandbox has no setup), not a bindings error.
        self.assertIn("setup", r.stdout + r.stderr)
        self.assertNotIn("GCPadNew", r.stdout + r.stderr)

    def test_live_playback_still_needs_pad_bindings(self):
        root = self.sandbox()
        (self.user / "Config" / "GCPadNew.ini").unlink()
        r = dev(root, "run", "--scenario", "s", "--live")
        self.assert_failed_before_building(r)
        self.assertIn("GCPadNew", r.stdout + r.stderr)


if __name__ == "__main__":
    unittest.main()
