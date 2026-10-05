"""Helpers for driving the real dev.py command line from tests.

Tests only observe exit codes, printed output and files on disk.
"""

import hashlib
import os
import shutil
import subprocess
import sys
import tempfile
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
