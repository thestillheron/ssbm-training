"""Live piloting: send a `tools.pad` plan to a window, guarded by focus.

No Win32 here. The backend (see `tools.win_pilot.Win32Backend`) supplies
`foreground()`, `focus(hwnd)` and `key(key, down)`, so the guard and release
logic can be tested with a fake.
"""

import time

from tools.pad import PadError


def run_plan(events, hwnd, backend, sleep=time.sleep, clock=time.monotonic):
    """Play `events` at their offsets in window `hwnd`, then release every held
    key and restore the previously focused window, on success and on every
    failure path (including Ctrl+C).

    Focus is taken only when a key event is due (never during an idle wait).
    Before each key event `hwnd` must be the foreground window: if it is not
    and no key is held it is brought back to the front; if keys are held they
    are released and PadError("focus lost ...") is raised.
    """
    previous = None  # window to restore; set when we take focus
    took_focus = False
    held = {}  # control -> Key
    try:
        start = clock()
        for ev in events:
            wait = ev.ms / 1000.0 - (clock() - start)
            if wait > 0:
                sleep(wait)
            if backend.foreground() != hwnd:
                if held:
                    raise PadError(
                        f"focus lost: Dolphin is no longer the foreground window "
                        f"(before {ev.action} {ev.control}); released all keys"
                    )
                previous = backend.foreground()
                took_focus = True
                backend.focus(hwnd)
                if backend.foreground() != hwnd:
                    raise PadError("could not bring the Dolphin window to the front")
            backend.key(ev.key, ev.action == "down")
            if ev.action == "down":
                held[ev.control] = ev.key
            else:
                held.pop(ev.control, None)
    finally:
        try:
            _release_all(held, backend)
        finally:
            if took_focus and previous and previous != hwnd:
                backend.focus(previous)


def _release_all(held, backend):
    """Release every held key; one failing release does not stop the rest."""
    errors = []
    for control, key in list(held.items()):
        try:
            backend.key(key, False)
        except Exception as e:
            errors.append(f"{control} ({key.name}): {e}")
        held.pop(control, None)
    if errors:
        raise PadError("could not release held keys: " + "; ".join(errors))
