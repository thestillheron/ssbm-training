"""Helpers for driving the real dev.py command line from tests.

Tests only observe exit codes, printed output and files on disk.
"""

import hashlib
import json
import os
import shlex
import shutil
import stat
import subprocess
import sys
import tempfile
import time
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
MAIN_DOL_SHA1 = "08e0bf20134dfcb260699671004527b2d6bb1a45"
DISC_EXTS = (".iso", ".rvz", ".gcm", ".wia", ".ciso", ".gcz", ".nfs")


def find_disc():
    """Disc image for disc-dependent tests: $SSBM_TEST_DISC, else orig/GALE01."""
    env = os.environ.get("SSBM_TEST_DISC")
    if env:
        p = Path(env)
        return p if p.is_file() else None
    folder = REPO_ROOT / "orig" / "GALE01"
    found = [p for p in folder.glob("*") if p.suffix.lower() in DISC_EXTS]
    return found[0] if len(found) == 1 else None


requires_disc = unittest.skipUnless(
    find_disc(),
    "no disc image: set SSBM_TEST_DISC or put one in orig/GALE01/",
)


def sha1(path):
    h = hashlib.sha1()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def dev(root, *args, timeout=3600):
    """Run `python dev.py <args>` in the given repo root."""
    return subprocess.run(
        [sys.executable, str(Path(root) / "dev.py"), *args],
        cwd=root,
        capture_output=True,
        text=True,
        timeout=timeout,
    )


CONFIG = REPO_ROOT / "dev.config.json"


def make_sandbox():
    """A minimal copy of the repo, enough to run `setup` in isolation."""
    root = Path(tempfile.mkdtemp(prefix="ssbm-dev-"))
    shutil.copy(REPO_ROOT / "dev.py", root)
    shutil.copy(REPO_ROOT / "flake.lock", root)
    shutil.copytree(REPO_ROOT / "reqs", root / "reqs")
    shutil.copytree(REPO_ROOT / "config" / "GALE01", root / "config" / "GALE01")
    (root / "tools").mkdir()
    shutil.copy(REPO_ROOT / "tools" / "download_tool.py", root / "tools")
    (root / "orig" / "GALE01" / "sys").mkdir(parents=True)
    (root / "orig" / "GALE01" / "files").mkdir()
    return root


def reset_orig(root):
    orig = Path(root) / "orig" / "GALE01"
    for sub in ("sys", "files"):
        shutil.rmtree(orig / sub, ignore_errors=True)
        (orig / sub).mkdir()
    for p in orig.glob(".extract*"):
        shutil.rmtree(p, ignore_errors=True)
    for p in orig.glob("*"):
        if p.is_file():
            p.unlink()


def make_full_sandbox():
    """A copy of the repo's tracked files (with their current working-tree
    contents), for tests that build. The caller deletes it."""
    root = Path(tempfile.mkdtemp(prefix="ssbm-chk-"))
    files = subprocess.run(
        ["git", "ls-files", "-z"],
        cwd=REPO_ROOT,
        capture_output=True,
        text=True,
        check=True,
    ).stdout.split("\0")
    for rel in filter(None, files):
        src = REPO_ROOT / rel
        if src.is_file():
            (root / rel).parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(src, root / rel)
    return root


CARD_BYTES = b"unlocked-everything" * 1000


def write_dolphin_ini(user, slot_a="1", card=None):
    """Dolphin.ini with slot A set to `slot_a` (1 = raw card file, 8 = GCI
    folder, 255 = none) and MemcardAPath `card` (forward slashes, as Dolphin
    writes it)."""
    lines = ["[Core]", "SIDevice0 = 6"]
    if card is not None:
        lines.append(f"MemcardAPath = {Path(card).as_posix()}")
    if slot_a is not None:
        lines.append(f"SlotA = {slot_a}")
    lines.append("SlotB = 255")
    (Path(user) / "Config").mkdir(parents=True, exist_ok=True)
    (Path(user) / "Config" / "Dolphin.ini").write_text("\n".join(lines) + "\n")


def fixture_card(user, data=CARD_BYTES):
    card = Path(user) / "GC" / "ssbm.USA.raw"
    card.parent.mkdir(parents=True, exist_ok=True)
    card.write_bytes(data)
    return card


# ------------------------------------------------------------ piloting sandboxes

PAD_INI = """[GCPad1]
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

# The tools/ modules behind pad, shot, scenario and run --scenario.
PILOTING_MODULES = (
    "__init__.py",
    "pad.py",
    "pad_live.py",
    "win_pilot.py",
    "scenario.py",
    "scenario_live.py",
    "shot.py",
    "movie.py",
)


def make_piloting_sandbox(scenarios=None, pad_ini=PAD_INI):
    """A copy of dev.py and the piloting modules, with `dolphin_user` in its
    dev.config.json pointing at a fixture Dolphin user folder (with
    `pad_ini` as its GCPadNew.ini, or none if None) and `scenarios`
    ({name: text}) in tools/scenarios. Returns (root, user); the caller
    deletes root."""
    root = Path(tempfile.mkdtemp(prefix="ssbm-pilot-"))
    shutil.copy(REPO_ROOT / "dev.py", root)
    (root / "tools").mkdir()
    for name in PILOTING_MODULES:
        shutil.copy(REPO_ROOT / "tools" / name, root / "tools")
    user = root / "fixture-user"
    (user / "Config").mkdir(parents=True)
    if pad_ini is not None:
        (user / "Config" / "GCPadNew.ini").write_text(pad_ini)
    (root / "dev.config.json").write_text(json.dumps({"dolphin_user": str(user)}))
    if scenarios is not None:
        folder = root / "tools" / "scenarios"
        folder.mkdir()
        for name, text in scenarios.items():
            (folder / f"{name}.txt").write_text(text)
    return root, user


def update_config(path, **keys):
    """Set `keys` in the dev.config.json at `path`."""
    path = Path(path)
    cfg = json.loads(path.read_text()) if path.exists() else {}
    cfg.update(keys)
    path.write_text(json.dumps(cfg))


# ------------------------------------------------------------ fake Dolphin


def make_fake_dolphin(folder, movie_option=True):
    """An executable that records its arguments, one per line, to args.txt.
    Like a real Dolphin it mentions its `--movie` option, unless
    `movie_option` is False (a Dolphin that cannot play movies). Returns
    (executable, args file)."""
    folder = Path(folder)
    out = folder / "args.txt"
    usage = "-m, --movie  Play a movie file" if movie_option else "no movies here"
    if os.name == "nt":
        # cmd's own argument splitting breaks at "=", so Python records them.
        script = folder / "fake_dolphin.py"
        script.write_text(
            f"import sys\nopen({str(out)!r}, 'w').write(''.join(a + '\\n' for a in sys.argv[1:]))\n"
        )
        exe = folder / "fake_dolphin.bat"
        exe.write_text(f'@echo off\r\nrem {usage}\r\n"{sys.executable}" "{script}" %*\r\n')
    else:
        exe = folder / "fake_dolphin.sh"
        exe.write_text(
            f"#!/bin/sh\n# {usage}\n"
            f'printf "%s\\n" "$@" > {shlex.quote(str(out))}\n'
        )
        exe.chmod(exe.stat().st_mode | stat.S_IEXEC)
    return exe, out


def launched_args(case, args_file, timeout=20):
    """The arguments the fake Dolphin was launched with (it is detached, so
    wait for its args file); fails `case` if it never was."""
    deadline = time.time() + timeout
    while time.time() < deadline and not args_file.exists():
        time.sleep(0.2)
    case.assertTrue(args_file.exists(), "fake Dolphin was never launched")
    time.sleep(0.5)  # let it finish writing
    return [a for a in args_file.read_text().split("\n") if a]


def make_run_sandbox(scenarios, movie_option=True):
    """A piloting sandbox for `run --scenario` checks that happen before the
    build: a fake Dolphin configured as `dolphin`. No setup, so a run that
    gets past its checks stops at the build. Returns (root, user, args file)."""
    root, user = make_piloting_sandbox(scenarios)
    exe, args_file = make_fake_dolphin(root, movie_option)
    update_config(root / "dev.config.json", dolphin=str(exe))
    return root, user, args_file


class FakeDolphinCase(unittest.TestCase):
    """Swaps a fake Dolphin into the real checkout's dev.config.json and
    restores the config afterwards."""

    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(prefix="ssbm-fake-dolphin-")
        self.addCleanup(self.tmp.cleanup)
        self.old_config = CONFIG.read_text() if CONFIG.exists() else None
        self.addCleanup(self.restore_config)

    def restore_config(self):
        if self.old_config is None:
            CONFIG.unlink(missing_ok=True)
        else:
            CONFIG.write_text(self.old_config)

    def configure_dolphin(self, path):
        cfg = json.loads(self.old_config) if self.old_config else {}
        cfg["dolphin"] = str(path)
        CONFIG.write_text(json.dumps(cfg))


@requires_disc
class RunScenarioCase(FakeDolphinCase):
    """`run --scenario` in the real checkout (it builds), up to the launch:
    a fake Dolphin that never opens a game window, so the command then fails
    waiting for it."""

    def setUp(self):
        super().setUp()
        r = dev(REPO_ROOT, "setup", str(find_disc()))
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        exe, self.args_file = make_fake_dolphin(self.tmp.name)
        self.configure_dolphin(exe)
        update_config(CONFIG, dolphin_window_title="^no-such-window-title-xyzzy$")

    def launched_args(self):
        return launched_args(self, self.args_file)
