#!/usr/bin/env python3
"""Developer entry point for this fork: setup and build on macOS and Windows.

Standard library only, so it runs before the project virtual environment
exists. Run it as `python dev.py <command>` (`py dev.py <command>` on Windows).

Commands:
  setup [DISC]       Create the virtual environment, extract the disc image
                     (RVZ or ISO) into orig/GALE01 and verify main.dol.
  build [--matching] Build the training build (build/training), or with
                     --matching the byte-identical matching build.
  run                Build the training build, assemble a game folder from it
                     and boot it in Dolphin.
  pad SEQUENCE       Piloting: send GameCube controls to the running training
                     build as keystrokes (--dry-run prints the plan only).
  shot [LABEL]       Screenshot the running training build with Dolphin's
                     hotkey; prints the PNG path (build/training/shots).
  scenario NAME      Play a committed scenario from tools/scenarios
                     (--dry-run prints the expanded plan only).
  check              Pre-merge gate: the matching build must pass the SHA-1
                     verification and the training build must compile and link.

Per-machine overrides live in the gitignored `dev.config.json`, e.g.
  {"disc": "D:/games/Melee.rvz", "wine": "/opt/homebrew/bin/wine",
   "dolphin": "C:/Tools/Dolphin/Dolphin.exe",
   "dolphin_user": "C:/Tools/Dolphin/User",
   "dolphin_window_title": "^Dolphin .*\\|"}
dolphin_user is Dolphin's user folder (where pad reads the bindings);
dolphin_window_title is a regex for Dolphin's game window title (default
"^Dolphin .*\\|"). setup also records what it auto-detected under "detected".
"""

import argparse
import hashlib
import json
import os
import shutil
import stat
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
VERSION = "GALE01"
IS_WINDOWS = os.name == "nt"
IS_MAC = sys.platform == "darwin"

CONFIG_FILE = ROOT / "dev.config.json"
VENV = ROOT / ".venv"
VENV_BIN = VENV / ("Scripts" if IS_WINDOWS else "bin")
VENV_PYTHON = VENV_BIN / ("python.exe" if IS_WINDOWS else "python")
VENV_NINJA = VENV_BIN / ("ninja.exe" if IS_WINDOWS else "ninja")
VENV_STAMP = VENV / ".dev_reqs_sha1"
DTK = ROOT / "build" / "tools" / ("dtk.exe" if IS_WINDOWS else "dtk")
ORIG = ROOT / "orig" / VERSION
BUILD_STATE = ROOT / "build" / "dev_state.json"
MATCHING_DOL = ROOT / "build" / VERSION / "main.dol"
TRAINING_DIR = ROOT / "build" / "training"
TRAINING_DOL = TRAINING_DIR / "main.dol"
GAME_DIR = TRAINING_DIR / "game"
SHOTS_DIR = TRAINING_DIR / "shots"
WINDOW_TIMEOUT = 30.0  # seconds `run --scenario` waits for the game window
DISC_EXTS ={".iso", ".rvz", ".gcm", ".wia", ".ciso", ".gcz", ".nfs"}


class DevError(Exception):
    pass


def say(msg=""):
    print(msg, flush=True)


def sha1_file(path):
    h = hashlib.sha1()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def load_config():
    if not CONFIG_FILE.is_file():
        return {}
    try:
        return json.loads(CONFIG_FILE.read_text(encoding="utf-8"))
    except ValueError as e:
        raise DevError(f"{CONFIG_FILE.name} is not valid JSON: {e}")


def expected_main_dol_sha1():
    line = (ROOT / "config" / VERSION / "build.sha1").read_text().split()
    return line[0]


def run(cmd, **kw):
    return subprocess.run([str(c) for c in cmd], cwd=ROOT, **kw)


# ---------------------------------------------------------------- setup


def find_disc(arg):
    if arg:
        p = Path(arg).expanduser()
        if not p.is_file():
            raise DevError(f"disc image not found: {p}")
        return p
    cfg = load_config().get("disc")
    if cfg:
        p = Path(cfg).expanduser()
        if not p.is_file():
            raise DevError(f"disc image from {CONFIG_FILE.name} not found: {p}")
        return p
    found = sorted(
        p for p in ORIG.glob("*") if p.is_file() and p.suffix.lower() in DISC_EXTS
    )
    if len(found) == 1:
        return found[0]
    if len(found) > 1:
        names = ", ".join(p.name for p in found)
        raise DevError(
            f"more than one disc image in {ORIG} ({names}). Leave just one "
            "there, or pass one explicitly: python dev.py setup <disc image>"
        )
    return None


def disc_is_extracted():
    main_dol = ORIG / "sys" / "main.dol"
    files = ORIG / "files"
    return (
        main_dol.is_file()
        and files.is_dir()
        and any(files.iterdir())
        and sha1_file(main_dol) == expected_main_dol_sha1()
    )


def ensure_dtk():
    if DTK.is_file():
        return
    tag = json.loads((ROOT / "flake.lock").read_text(encoding="utf-8"))["nodes"][
        "decomp-toolkit"
    ]["original"]["ref"]
    say(f"Downloading decomp-toolkit {tag} ...")
    DTK.parent.mkdir(parents=True, exist_ok=True)
    r = run([sys.executable, "tools/download_tool.py", "dtk", DTK, "--tag", tag])
    if r.returncode != 0 or not DTK.is_file():
        raise DevError("could not download decomp-toolkit")


def manual_extraction_help():
    return (
        "Automatic extraction failed. Fallback: open the image in Dolphin, "
        "right-click it in the game list, choose Properties > Filesystem, "
        "right-click 'Disc', pick 'Extract Entire Disc...' and extract the "
        f"whole disc into {ORIG}. Then run setup again."
    )


def extract_disc(disc):
    tmp = ORIG / ".extract-tmp"
    shutil.rmtree(tmp, ignore_errors=True)
    say(f"Extracting {disc} ...")
    try:
        r = run([DTK, "disc", "extract", "-q", disc, tmp])
        if r.returncode != 0:
            raise DevError(
                f"could not extract {disc} (corrupt or unsupported image?)\n"
                + manual_extraction_help()
            )
        main_dol = tmp / "sys" / "main.dol"
        if not main_dol.is_file() or not (tmp / "files").is_dir():
            raise DevError(f"{disc} did not extract to a full disc")
        if sha1_file(main_dol) != expected_main_dol_sha1():
            raise DevError(
                f"{disc} is not the right game: main.dol does not match the "
                f"recorded SHA-1. The game must be Melee NTSC-U 1.02 "
                f"({VERSION}); other regions and versions are not supported."
            )
        # Verified: swap it in. Keep tracked placeholders such as sys/.gitkeep.
        for sub in ("sys", "files"):
            dest = ORIG / sub
            dest.mkdir(parents=True, exist_ok=True)
            for old in dest.iterdir():
                if old.name == ".gitkeep":
                    continue
                shutil.rmtree(old) if old.is_dir() else old.unlink()
            for new in (tmp / sub).iterdir():
                shutil.move(str(new), str(dest / new.name))
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def ensure_venv():
    reqs = (ROOT / "reqs" / "build.txt").read_bytes()
    stamp = hashlib.sha1(reqs).hexdigest()
    if (
        VENV_PYTHON.is_file()
        and VENV_NINJA.is_file()
        and VENV_STAMP.is_file()
        and VENV_STAMP.read_text() == stamp
    ):
        return
    say("Setting up the virtual environment (.venv) ...")
    if not VENV_PYTHON.is_file():
        r = run([sys.executable, "-m", "venv", VENV])
        if r.returncode != 0:
            raise DevError("could not create the virtual environment")
    r = run(
        [VENV_PYTHON, "-m", "pip", "install", "-q", "ninja", "-r", "reqs/build.txt"]
    )
    if r.returncode != 0:
        raise DevError("pip install failed")
    VENV_STAMP.write_text(stamp)


def find_wine():
    cfg = load_config().get("wine")
    if cfg:
        return Path(cfg).expanduser() if Path(cfg).expanduser().is_file() else None
    found = shutil.which("wine")
    if found:
        return Path(found)
    for p in ("/opt/homebrew/bin/wine", "/usr/local/bin/wine"):
        if Path(p).is_file():
            return Path(p)
    return None


def find_dolphin():
    """Dolphin executable: the config `dolphin` key, else the usual install
    locations for this OS. Returns None if not found (a configured path that
    does not exist is not silently replaced by auto-detection)."""
    cfg = load_config().get("dolphin")
    if cfg:
        p = Path(cfg).expanduser()
        return p if p.is_file() else None
    candidates = []
    if IS_MAC:
        for base in (Path("/Applications"), Path.home() / "Applications"):
            candidates.append(base / "Dolphin.app" / "Contents" / "MacOS" / "Dolphin")
    elif IS_WINDOWS:
        for var in ("ProgramFiles", "ProgramFiles(x86)", "LOCALAPPDATA", "APPDATA"):
            base = os.environ.get(var)
            if not base:
                continue
            for sub in (
                "Dolphin-x64",
                "Dolphin",
                "Dolphin Emulator",
                "Programs/Dolphin-x64",
                "Programs/Dolphin",
            ):
                candidates.append(Path(base) / sub / "Dolphin.exe")
        candidates.append(Path.home() / "Dolphin-x64" / "Dolphin.exe")
    for p in candidates:
        if p.is_file():
            return p
    for name in ("Dolphin", "dolphin-emu"):
        found = shutil.which(name)
        if found:
            return Path(found)
    return None


DOLPHIN_HELP = (
    "Dolphin not found (needed later to run the game, not for setup). Install "
    "it from https://dolphin-emu.org/download/"
    + (" or `brew install --cask dolphin`" if IS_MAC else "")
    + f", or set \"dolphin\" in {CONFIG_FILE.name}."
)


def record_detected(found):
    """Remember what was auto-detected in the per-machine config. Only touches
    the file when something changed; never edits override keys."""
    found = {k: str(v) for k, v in found.items() if v}
    if not found:
        return
    cfg = load_config()
    detected = dict(cfg.get("detected", {}))
    if all(detected.get(k) == v for k, v in found.items()):
        return
    detected.update(found)
    cfg["detected"] = detected
    CONFIG_FILE.write_text(json.dumps(cfg, indent=2) + "\n", encoding="utf-8")


def check_tools():
    """Detect Wine (required on macOS, or wherever configured) and Dolphin
    (optional). Prints what it found and records it in the config."""
    wine_wanted = IS_MAC or "wine" in load_config()
    wine = find_wine() if wine_wanted else None
    dolphin = find_dolphin()
    record_detected({"wine": wine, "dolphin": dolphin})
    if dolphin:
        say(f"Dolphin: {dolphin}")
    else:
        say(DOLPHIN_HELP)
    if wine_wanted:
        check_wine(wine)


def check_wine(wine):
    if wine:
        say(f"Wine: {wine}")
        say(
            "  If wine stops launching after a macOS upgrade, clear its "
            "quarantine flag:\n    xattr -dr com.apple.quarantine "
            "/Applications/Wine*.app (adjust to your install)"
        )
        return
    raise DevError(
        "Wine is required on macOS to run the Windows-only MWCC compiler. "
        "Install it with:\n    brew install --cask --no-quarantine "
        "wine-stable\n(Apple Silicon also needs Rosetta: "
        "softwareupdate --install-rosetta). Or set \"wine\" in "
        f"{CONFIG_FILE.name}."
    )


def cmd_setup(args):
    check_tools()
    if disc_is_extracted() and not args.force:
        say("Disc already extracted and verified.")
    else:
        disc = find_disc(args.disc)
        if disc is None:
            raise DevError(
                "no disc image found. Pass one (RVZ or ISO): "
                f"python dev.py setup <disc image>, or put a single image in {ORIG}, "
                f"or set \"disc\" in {CONFIG_FILE.name}."
            )
        ensure_dtk()
        extract_disc(disc)
        say("Disc extracted; main.dol matches the recorded SHA-1.")
    ensure_venv()
    say("Setup complete.")


# ---------------------------------------------------------------- build


# mode -> (main.dol it produces, extra configure.py arguments)
MODES = {
    "matching": (MATCHING_DOL, []),
    "training": (TRAINING_DOL, ["--training"]),
}


def configure_py_fingerprint():
    st = (ROOT / "configure.py").stat()
    return [st.st_mtime_ns, st.st_size]


def load_state():
    try:
        state = json.loads(BUILD_STATE.read_text())
    except (OSError, ValueError):
        return {}
    return state if isinstance(state, dict) else {}


def save_state(**updates):
    """Merge `updates` into the dev state file (other keys are kept)."""
    state = load_state()
    state.update(updates)
    BUILD_STATE.parent.mkdir(parents=True, exist_ok=True)
    BUILD_STATE.write_text(json.dumps(state))


def configured_for(mode):
    state = load_state()
    return (
        state.get("mode") == mode
        and state.get("configure_py") == configure_py_fingerprint()
        and (ROOT / "build.ninja").is_file()
    )


def configure(mode):
    cmd = [VENV_PYTHON, "configure.py", *MODES[mode][1]]
    if IS_MAC:
        wine = find_wine()
        if wine:
            cmd += ["--wrapper", wine]
    say("Configuring ...")
    if run(cmd).returncode != 0:
        raise DevError("configure.py failed")
    save_state(mode=mode, configure_py=configure_py_fingerprint())


def build(mode):
    """Build `mode` ("matching" or "training") and return its main.dol."""
    if not VENV_NINJA.is_file() or not (ORIG / "sys" / "main.dol").is_file():
        raise DevError("run `python dev.py setup <disc image>` first")
    dol = MODES[mode][0]
    if not configured_for(mode):
        configure(mode)
    say(f"Building the {mode} build ...")
    if run([VENV_NINJA]).returncode != 0:
        raise DevError(f"{mode} build failed")
    if not dol.is_file():
        raise DevError(f"build finished but {dol} was not produced")
    say(f"{mode.capitalize()} build OK: {dol.relative_to(ROOT)}")
    return dol


def cmd_build(args):
    build("matching" if args.matching else "training")


# ---------------------------------------------------------------- run


def link_or_copy(src, dest):
    """Hard-link src to dest, falling back to a copy. True if it was linked."""
    try:
        os.link(src, dest)
        return True
    except OSError:
        shutil.copy2(src, dest)
        return False


def _clear_readonly_and_retry(func, path, _exc):
    os.chmod(path, stat.S_IWRITE)
    func(path)


def remove_tree(path):
    """Delete a directory tree, failing loudly rather than leaving stale files."""
    if not path.exists():
        return
    try:
        if sys.version_info >= (3, 12):
            shutil.rmtree(path, onexc=_clear_readonly_and_retry)
        else:
            shutil.rmtree(path, onerror=_clear_readonly_and_retry)
    except OSError as e:
        raise DevError(
            f"could not remove the old game folder {path}: {e}\n"
            "Close Dolphin (or anything else using it) and try again."
        )


def assemble_game(dol):
    """Mirror the extracted disc into GAME_DIR using links, with `dol` as main.dol.

    Nothing under orig/ is ever written: main.dol in the game folder is a fresh
    copy (never a link to the original), and everything else is read-only linked.
    """
    remove_tree(GAME_DIR)
    copied = 0
    for src in sorted(p for p in ORIG.rglob("*") if p.is_file()):
        rel = src.relative_to(ORIG)
        if rel.parts[0] not in ("sys", "files") or src.name == ".gitkeep":
            continue
        dest = GAME_DIR / rel
        dest.parent.mkdir(parents=True, exist_ok=True)
        if rel == Path("sys/main.dol"):
            shutil.copyfile(dol, dest)
        elif not link_or_copy(src, dest):
            copied += 1
    if copied:
        say(
            f"warning: could not hard-link {copied} files (different drive or "
            "filesystem?); copied them instead, so this run was slower"
        )
    return GAME_DIR / "sys" / "main.dol"


def cmd_run(args):
    events = None
    if args.scenario:
        require_piloting_platform()
        events = load_scenario_plan(args.scenario)  # fail before building
    dolphin = find_dolphin()
    if dolphin is None:
        raise DevError(
            "could not find Dolphin. Set \"dolphin\" in "
            f"{CONFIG_FILE.name} to its executable, e.g. "
            '{"dolphin": "C:/path/to/Dolphin.exe"}'
        )
    dol = build("training")
    stop_tracked_dolphin(load_state())
    from tools import shot

    shot.clear_shots(SHOTS_DIR)
    game_dol = assemble_game(dol)
    say(f"Launching {dolphin.name} ...")
    kw = {}
    if IS_WINDOWS:
        kw["creationflags"] = subprocess.DETACHED_PROCESS | subprocess.CREATE_NEW_PROCESS_GROUP
    else:
        kw["start_new_session"] = True
    proc = subprocess.Popen(
        [str(dolphin), "-e", str(game_dol)],
        stdin=subprocess.DEVNULL,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        **kw,
    )
    save_state(dolphin_pid=proc.pid)
    if events is not None:
        say(f"Waiting for the game window (up to {WINDOW_TIMEOUT:g}s) ...")
        play_live(events, wait=True)


def stop_tracked_dolphin(state, image_of=None, terminate=None):
    """Terminate the Dolphin that the last `run` launched (state key
    `dolphin_pid`), but only if that PID is still a Dolphin process. Other
    Dolphin processes are never touched. Windows only; elsewhere a no-op."""
    pid = state.get("dolphin_pid")
    if not pid:
        return
    if image_of is None or terminate is None:
        if not IS_WINDOWS:
            return
        from tools import win_pilot

        image_of = image_of or win_pilot.process_image
        terminate = terminate or win_pilot.terminate
    image = image_of(pid)
    if image and Path(image).stem.lower() == "dolphin":
        say(f"Stopping the Dolphin from the last run (pid {pid}) ...")
        try:
            terminate(pid)
        except OSError as e:
            raise DevError(f"could not stop the previous Dolphin (pid {pid}): {e}")


# ---------------------------------------------------------------- pad


def dolphin_user_dir():
    """Dolphin's user folder: the config `dolphin_user` key, else `User` beside
    a configured/found Dolphin that has portable.txt, else the per-user
    location."""
    override = load_config().get("dolphin_user")
    if override:
        return Path(override).expanduser()
    dolphin = find_dolphin()
    if dolphin is not None and (dolphin.parent / "portable.txt").exists():
        return dolphin.parent / "User"
    if IS_WINDOWS:
        return Path(os.environ.get("APPDATA", Path.home() / "AppData" / "Roaming")) / "Dolphin Emulator"
    if IS_MAC:
        return Path.home() / "Library" / "Application Support" / "Dolphin"
    return Path.home() / ".local" / "share" / "dolphin-emu"


def require_piloting_platform():
    if not IS_WINDOWS:
        raise DevError("piloting is not supported yet on this OS (Windows only)")


def cmd_pad(args):
    from tools import pad

    require_piloting_platform()
    try:
        steps = pad.parse_sequence(args.sequence)
        bindings = pad.load_bindings(pad.pad_config_path(dolphin_user_dir()))
        events = pad.build_plan(steps, bindings)
    except pad.PadError as e:
        raise DevError(str(e))
    if args.dry_run:
        say(pad.format_plan(events))
        return
    from tools import pad_live, win_pilot

    try:
        pad_live.run_plan(events, find_game_window(), win_pilot.Win32Backend())
    except pad.PadError as e:
        raise DevError(str(e))


def find_game_window():
    """hwnd of the running Dolphin game window (Windows only); PadError if
    there is none or several."""
    from tools import win_pilot

    return win_pilot.find_dolphin_window(
        load_state().get("dolphin_pid"), load_config().get("dolphin_window_title")
    )


def cmd_shot(args):
    from tools import pad, shot

    require_piloting_platform()
    user = dolphin_user_dir()
    try:
        key = shot.screenshot_key(user)
        target = shot.shot_target(args.label, SHOTS_DIR)
        if args.dry_run:
            say(f"hotkey: {key.name} (scancode 0x{key.scancode:02X})")
            say(f"Dolphin writes to: {shot.screenshots_dir(user)}")
            say(f"shot lands at: {target}")
            return
        from tools import win_pilot

        hwnd = find_game_window()
        path = shot.take_shot(
            args.label,
            key,
            hwnd,
            win_pilot.Win32Backend(),
            shot.screenshots_dir(user),
            SHOTS_DIR,
            timeout=args.timeout,
        )
    except pad.PadError as e:
        raise DevError(str(e))
    say(str(path))


def load_scenario_plan(name):
    """Expand scenario `name` into a timed plan, or raise DevError."""
    from tools import pad, scenario

    try:
        items = scenario.expand(name)
        bindings = pad.load_bindings(pad.pad_config_path(dolphin_user_dir()))
        return scenario.build_plan(items, bindings)
    except (pad.PadError, scenario.ScenarioError) as e:
        raise DevError(str(e))


def play_live(events, wait=False):
    """Play a scenario plan against the Dolphin window, taking its shots.
    With `wait`, first wait (up to WINDOW_TIMEOUT s) for the window to appear;
    nothing is sent if it doesn't."""
    from tools import pad, scenario_live, shot, win_pilot

    user = dolphin_user_dir()
    try:
        key = shot.screenshot_key(user)

        hwnd = scenario_live.wait_for_window(find_game_window, WINDOW_TIMEOUT if wait else 0)
        backend = win_pilot.Win32Backend()

        def take(label):
            return shot.take_shot(
                label, key, hwnd, backend, shot.screenshots_dir(user), SHOTS_DIR
            )

        scenario_live.play(events, hwnd, backend, take, say=say)
    except pad.PadError as e:
        raise DevError(str(e))


def cmd_scenario(args):
    require_piloting_platform()
    from tools import scenario

    events = load_scenario_plan(args.name)
    if args.dry_run:
        say(scenario.format_plan(events))
        return
    play_live(events)


# ---------------------------------------------------------------- check


def cmd_check(args):
    """Build both builds (even if the first fails) and summarise. The matching
    build's SHA-1 verification is ninja's own `check` step, run by build()."""
    results = {}
    for mode in ("matching", "training"):
        try:
            build(mode)
            results[mode] = None
        except DevError as e:
            results[mode] = str(e)
    say()
    say("check summary")
    for mode, err in results.items():
        say(f"{mode}: {'PASS' if err is None else 'FAIL'}")
    failed = {m: e for m, e in results.items() if e is not None}
    if failed:
        raise DevError("; ".join(f"{m}: {e}" for m, e in failed.items()))


# ---------------------------------------------------------------- main


def main(argv=None):
    p = argparse.ArgumentParser(prog="dev.py", description=__doc__.split("\n\n")[0])
    sub = p.add_subparsers(dest="command", required=True)
    s = sub.add_parser("setup", help="prepare this machine from a fresh clone")
    s.add_argument("disc", nargs="?", help="Melee NTSC-U 1.02 image (RVZ or ISO)")
    s.add_argument("--force", action="store_true", help="re-extract the disc")
    s.set_defaults(func=cmd_setup)
    b = sub.add_parser("build", help="build the game")
    b.add_argument("--matching", action="store_true", help="build the matching build")
    b.set_defaults(func=cmd_build)
    r = sub.add_parser("run", help="build the training build and boot it in Dolphin")
    r.add_argument(
        "--scenario",
        metavar="NAME",
        help="after launching, wait for the game window and play this scenario",
    )
    r.set_defaults(func=cmd_run)
    pd = sub.add_parser(
        "pad", help="pilot the training build: send GameCube controls as keystrokes"
    )
    pd.add_argument("sequence", help='steps, e.g. "start wait:30 stick-down+b:6"')
    pd.add_argument(
        "--dry-run", action="store_true", help="print the timed key plan, send nothing"
    )
    pd.set_defaults(func=cmd_pad)
    sh = sub.add_parser(
        "shot", help="screenshot the running training build into build/training/shots"
    )
    sh.add_argument("label", nargs="?", help="file name (without .png); default a timestamp")
    sh.add_argument(
        "--timeout", type=float, default=10.0, help="seconds to wait for Dolphin's file"
    )
    sh.add_argument(
        "--dry-run", action="store_true", help="print the hotkey and paths, send nothing"
    )
    sh.set_defaults(func=cmd_shot)
    sc = sub.add_parser("scenario", help="play a committed piloting scenario")
    sc.add_argument("name", help="scenario name (file stem in tools/scenarios)")
    sc.add_argument(
        "--dry-run", action="store_true", help="print the expanded timed plan only"
    )
    sc.set_defaults(func=cmd_scenario)
    c = sub.add_parser("check", help="pass/fail gate for both builds")
    c.set_defaults(func=cmd_check)
    args = p.parse_args(argv)
    try:
        args.func(args)
    except DevError as e:
        print(f"error: {e}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
