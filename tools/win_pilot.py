"""Windows-only plumbing for piloting: window lookup, foreground control,
SendInput scancodes and process helpers. Kept thin and logic-free; the guard
logic lives in `tools.pad_live`. Do not run any of this from automated tests
against a real Dolphin: it takes focus and sends keystrokes.
"""

import ctypes
import os
import re
import time
from ctypes import wintypes

from tools.pad import PadError

# The title assumption: Dolphin's render/main window title starts with
# "Dolphin" (e.g. "Dolphin 2503 | JIT64 DC | OpenGL | HLE | ..."). Override
# with "dolphin_window_title" (a regex) in dev.config.json.
# The game window carries "| backend | game" in its title; the bare "Dolphin 2609" window is the main frame.
DEFAULT_TITLE_PATTERN = r"^Dolphin .*\|"

user32 = ctypes.WinDLL("user32", use_last_error=True)
kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)

INPUT_KEYBOARD = 1
KEYEVENTF_EXTENDEDKEY = 0x0001
KEYEVENTF_KEYUP = 0x0002
KEYEVENTF_SCANCODE = 0x0008
PROCESS_TERMINATE = 0x0001
PROCESS_QUERY_LIMITED_INFORMATION = 0x1000
ERROR_INVALID_PARAMETER = 87
SW_RESTORE = 9
VK_MENU = 0x12

ULONG_PTR = ctypes.c_size_t


class KEYBDINPUT(ctypes.Structure):
    _fields_ = [
        ("wVk", wintypes.WORD),
        ("wScan", wintypes.WORD),
        ("dwFlags", wintypes.DWORD),
        ("time", wintypes.DWORD),
        ("dwExtraInfo", ULONG_PTR),
    ]


class MOUSEINPUT(ctypes.Structure):
    _fields_ = [
        ("dx", wintypes.LONG),
        ("dy", wintypes.LONG),
        ("mouseData", wintypes.DWORD),
        ("dwFlags", wintypes.DWORD),
        ("time", wintypes.DWORD),
        ("dwExtraInfo", ULONG_PTR),
    ]


class _INPUTUNION(ctypes.Union):
    _fields_ = [("ki", KEYBDINPUT), ("mi", MOUSEINPUT)]


class INPUT(ctypes.Structure):
    _anonymous_ = ("u",)
    _fields_ = [("type", wintypes.DWORD), ("u", _INPUTUNION)]


# ------------------------------------------------------------- processes


def process_image(pid):
    """Full executable path of `pid`, or None if it isn't running."""
    h = kernel32.OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, False, pid)
    if not h:
        return None
    try:
        buf = ctypes.create_unicode_buffer(32768)
        size = wintypes.DWORD(len(buf))
        if kernel32.QueryFullProcessImageNameW(h, 0, buf, ctypes.byref(size)):
            return buf.value
        return None
    finally:
        kernel32.CloseHandle(h)


def terminate(pid):
    """Terminate `pid`. A process that is already gone is fine; any other
    failure raises OSError."""
    h = kernel32.OpenProcess(PROCESS_TERMINATE, False, pid)
    if not h:
        err = ctypes.get_last_error()
        if err == ERROR_INVALID_PARAMETER:  # no such process
            return
        raise ctypes.WinError(err)
    try:
        if not kernel32.TerminateProcess(h, 1):
            raise ctypes.WinError(ctypes.get_last_error())
        kernel32.WaitForSingleObject(h, 5000)
    finally:
        kernel32.CloseHandle(h)


# --------------------------------------------------------------- windows

_ENUMPROC = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)


def visible_windows():
    """[(hwnd, title, pid)] for visible top-level windows with a title."""
    found = []

    def cb(hwnd, _lparam):
        if user32.IsWindowVisible(hwnd):
            n = user32.GetWindowTextLengthW(hwnd)
            if n:
                buf = ctypes.create_unicode_buffer(n + 1)
                user32.GetWindowTextW(hwnd, buf, n + 1)
                pid = wintypes.DWORD()
                user32.GetWindowThreadProcessId(hwnd, ctypes.byref(pid))
                found.append((hwnd, buf.value, pid.value))
        return True

    user32.EnumWindows(_ENUMPROC(cb), 0)
    return found


def select_dolphin_window(windows, pid, pattern):
    """Pick the single Dolphin window from `windows` ([(hwnd, title, pid)]).

    Windows of process `pid` (the Dolphin `run` launched) whose title matches
    `pattern` win; if there are none, fall back to title matching across all
    windows. Zero or several matches raise PadError.
    """
    rx = re.compile(pattern, re.IGNORECASE)
    matches = [w for w in windows if rx.search(w[1])]
    own = [w for w in matches if pid is not None and w[2] == pid]
    chosen = own or matches
    if not chosen:
        raise PadError(
            f"no Dolphin window found (title pattern {pattern!r}); start the "
            "training build with `dev.py run`, or set \"dolphin_window_title\" "
            "(a regex) in dev.config.json to match your Dolphin window title"
        )
    if len(chosen) > 1:
        titles = "; ".join(f"{t!r} (pid {p})" for _h, t, p in chosen)
        raise PadError(
            f"{len(chosen)} Dolphin windows match {pattern!r}: {titles}. "
            "Close the extras, or narrow \"dolphin_window_title\" in dev.config.json"
        )
    return chosen[0][0]


def find_dolphin_window(pid, pattern):
    return select_dolphin_window(visible_windows(), pid, pattern or DEFAULT_TITLE_PATTERN)


# ------------------------------------------------------------ the backend


class Win32Backend:
    def foreground(self):
        return user32.GetForegroundWindow()

    def focus(self, hwnd):
        """Bring `hwnd` to the front. Tries thread-attached SetForegroundWindow
        first; if Windows' foreground lock still refuses, taps Alt (a synthetic
        key that does nothing on the current foreground window) and retries once."""
        if not hwnd:
            return
        if user32.IsIconic(hwnd):
            user32.ShowWindow(hwnd, SW_RESTORE)
        # Attach to the foreground thread so SetForegroundWindow is usually
        # allowed from a background process. If Windows still refuses, the
        # fallback below taps Alt (a synthetic key) to lift the lock.
        fg = user32.GetForegroundWindow()
        me = kernel32.GetCurrentThreadId()
        fg_thread = user32.GetWindowThreadProcessId(fg, None) if fg else 0
        attached = bool(
            fg_thread and fg_thread != me and user32.AttachThreadInput(me, fg_thread, True)
        )
        try:
            user32.SetForegroundWindow(hwnd)
        finally:
            if attached:
                user32.AttachThreadInput(me, fg_thread, False)
        if not self._wait_foreground(hwnd):
            # Windows' foreground lock refused (we did not get the last input).
            # A tap of Alt, which lands on the current foreground window and
            # does nothing there, lifts the lock for one SetForegroundWindow.
            user32.keybd_event(VK_MENU, 0, 0, 0)
            user32.keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0)
            user32.SetForegroundWindow(hwnd)
            self._wait_foreground(hwnd)

    @staticmethod
    def _wait_foreground(hwnd):
        for _ in range(20):  # wait up to ~200 ms for the switch
            if user32.GetForegroundWindow() == hwnd:
                return True
            time.sleep(0.01)
        return user32.GetForegroundWindow() == hwnd

    def key(self, key, down):
        self._raw(key.scancode, key.extended, down)

    def _raw(self, scancode, extended, down):
        flags = KEYEVENTF_SCANCODE
        if extended:
            flags |= KEYEVENTF_EXTENDEDKEY
        if not down:
            flags |= KEYEVENTF_KEYUP
        inp = INPUT(type=INPUT_KEYBOARD)
        inp.ki = KEYBDINPUT(0, scancode, flags, 0, 0)
        if user32.SendInput(1, ctypes.byref(inp), ctypes.sizeof(INPUT)) != 1:
            raise PadError(f"SendInput failed: {ctypes.get_last_error()}")
