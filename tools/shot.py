"""Piloting: screenshots of the running training build via Dolphin's own
screenshot hotkey.

No Win32 here; the live routine takes the same backend as
`tools.pad_live.run_plan`, so it is testable with a fake. Entry points:

  screenshot_key(user_dir)      -> Key     (Dolphin's hotkey config, else F9)
  screenshots_dir(user_dir)     -> Path    (where Dolphin writes its PNGs)
  shot_target(label, shots_dir) -> Path    (validated destination path)
  take_shot(label, key, hwnd, backend, screenshots, shots_dir) -> Path
  clear_shots(shots_dir)                   (what `run` does at each launch)

`take_shot` is the reusable shot-taking routine (scenario `shot` steps call
it too). It raises `tools.pad.PadError` on failure.
"""

import re
import shutil
import time
from pathlib import Path

from tools import pad, pad_live
from tools.pad import PadError

DEFAULT_HOTKEY = "F9"
HOTKEY_NAME = "General/Take Screenshot"
DEFAULT_TIMEOUT = 10.0
PRESS_MS = 50
POLL_S = 0.1

_LABEL_RE = re.compile(r"^[A-Za-z0-9][A-Za-z0-9._-]*$")


def hotkeys_path(user_dir):
    return Path(user_dir) / "Config" / "Hotkeys.ini"


def screenshots_dir(user_dir):
    return Path(user_dir) / "ScreenShots"


def screenshot_key(user_dir):
    """The keyboard key bound to Dolphin's "Take Screenshot" hotkey, read
    fresh from Hotkeys.ini; F9 (Dolphin's default) if it is not bound there."""
    path = hotkeys_path(user_dir)
    value = None
    if path.is_file():
        section = pad.read_section(path, "Hotkeys") or {}
        value = section.get(HOTKEY_NAME)
    if not value:
        value = DEFAULT_HOTKEY
    key = None if value.startswith("@(") or "+" in value else pad.key_from_name(value)
    if key is None:
        raise PadError(
            f"cannot send Dolphin's '{HOTKEY_NAME}' hotkey ({value}): only a "
            f"single keyboard key is supported. Bind it to one key (for "
            f"example {DEFAULT_HOTKEY}) in Dolphin's Hotkey settings."
        )
    return key


def shot_target(label, shots_dir):
    """Absolute destination for a shot: `<label>.png`, or a timestamp name."""
    shots_dir = Path(shots_dir)
    if label is None:
        stem = time.strftime("%Y%m%d-%H%M%S")
        target = shots_dir / f"{stem}.png"
        n = 1
        while target.exists():
            n += 1
            target = shots_dir / f"{stem}-{n}.png"
        return target.resolve()
    if not _LABEL_RE.match(label):
        raise PadError(
            f"bad shot label {label!r}: use letters, digits, '.', '_' and '-' only"
        )
    return (shots_dir / f"{label}.png").resolve()


def clear_shots(shots_dir):
    shots_dir = Path(shots_dir)
    if shots_dir.is_dir():
        shutil.rmtree(shots_dir, ignore_errors=True)


def _pngs(folder):
    folder = Path(folder)
    if not folder.is_dir():
        return {}
    return {p: p.stat().st_mtime_ns for p in folder.rglob("*.png")}


def _wait_for_new_png(folder, before, timeout, sleep, clock):
    deadline = clock() + timeout
    last = None
    while True:
        new = {p: m for p, m in _pngs(folder).items() if p not in before}
        if new:
            newest = max(new, key=new.get)
            size = newest.stat().st_size
            # Dolphin writes the file asynchronously: wait until it stops growing.
            if size > 0 and last == (newest, size):
                return newest
            last = (newest, size)
        if clock() >= deadline:
            raise PadError(
                f"no new screenshot appeared in {folder} within {timeout:g}s. "
                "Is the game running (`dev.py run`) and the screenshot hotkey "
                "bound in Dolphin?"
            )
        sleep(POLL_S)


def take_shot(
    label,
    key,
    hwnd,
    backend,
    screenshots,
    shots_dir,
    timeout=DEFAULT_TIMEOUT,
    sleep=time.sleep,
    clock=time.monotonic,
):
    """Press the screenshot hotkey in the Dolphin window `hwnd` (focus-guarded,
    via `pad_live.run_plan`), wait for Dolphin's new PNG in `screenshots`,
    move it to `shots_dir` as `<label>.png` (timestamp name if no label) and
    return its absolute path."""
    target = shot_target(label, shots_dir)
    before = _pngs(screenshots)
    events = [
        pad.KeyEvent(0, "down", "screenshot", key),
        pad.KeyEvent(PRESS_MS, "up", "screenshot", key),
    ]
    pad_live.run_plan(events, hwnd, backend, sleep=sleep, clock=clock)
    found = _wait_for_new_png(screenshots, before, timeout, sleep, clock)
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.move(str(found), str(target))
    return target
