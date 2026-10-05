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
  check              Pre-merge gate: the matching build must pass the SHA-1
                     verification and the training build must compile and link.

Per-machine overrides live in the gitignored `dev.config.json`, e.g.
  {"disc": "D:/games/Melee.rvz", "wine": "/opt/homebrew/bin/wine",
   "dolphin": "C:/Tools/Dolphin/Dolphin.exe"}
setup also records what it auto-detected under "detected".
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


def configured_for(mode):
    try:
        state = json.loads(BUILD_STATE.read_text())
    except (OSError, ValueError):
        return False
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
    BUILD_STATE.parent.mkdir(parents=True, exist_ok=True)
    BUILD_STATE.write_text(
        json.dumps({"mode": mode, "configure_py": configure_py_fingerprint()})
    )


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
    dolphin = find_dolphin()
    if dolphin is None:
        raise DevError(
            "could not find Dolphin. Set \"dolphin\" in "
            f"{CONFIG_FILE.name} to its executable, e.g. "
            '{"dolphin": "C:/path/to/Dolphin.exe"}'
        )
    dol = build("training")
    game_dol = assemble_game(dol)
    say(f"Launching {dolphin.name} ...")
    kw = {}
    if IS_WINDOWS:
        kw["creationflags"] = subprocess.DETACHED_PROCESS | subprocess.CREATE_NEW_PROCESS_GROUP
    else:
        kw["start_new_session"] = True
    subprocess.Popen(
        [str(dolphin), "-e", str(game_dol)],
        stdin=subprocess.DEVNULL,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        **kw,
    )


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
    r.set_defaults(func=cmd_run)
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
