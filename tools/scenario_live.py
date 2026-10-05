"""Live scenarios: play a `tools.scenario` plan against a running Dolphin.

No Win32 here. Key events go through `tools.pad_live.run_plan` (focus-guarded,
releases held keys on every failure); shot steps call the `take_shot`
callable. Waiting for the game window after `run` also lives here.
"""

import time

from tools import pad_live
from tools.pad import PadError
from tools.scenario import ShotEvent

POLL_S = 0.5


def wait_for_window(find, timeout, sleep=time.sleep, clock=time.monotonic):
    """Call `find()` (returns an hwnd or raises PadError) until it succeeds or
    `timeout` seconds pass; then raise a PadError that says so."""
    deadline = clock() + timeout
    while True:
        try:
            return find()
        except PadError as e:
            last = e
        if timeout <= 0:
            raise last
        if clock() >= deadline:
            raise PadError(
                f"the game window did not appear within {timeout:g}s ({last}). "
                "Nothing was sent."
            )
        sleep(POLL_S)


def play(events, hwnd, backend, take_shot, say=print, sleep=time.sleep, clock=time.monotonic):
    """Play a scenario plan ([KeyEvent | ShotEvent]). Runs of key events go
    through run_plan (so every abort releases keys and restores focus); idle
    time before a shot is waited out; `take_shot(label)` returns the path,
    which is passed to `say`."""
    segment = []
    base = 0  # plan time (ms) the current segment starts at

    def flush(until):
        nonlocal segment
        rebased = [e._replace(ms=e.ms - base) for e in segment]
        if rebased:
            pad_live.run_plan(rebased, hwnd, backend, sleep=sleep, clock=clock)
        idle = (until - base - (rebased[-1].ms if rebased else 0)) / 1000.0
        if idle > 0:
            sleep(idle)
        segment = []

    for ev in events:
        if isinstance(ev, ShotEvent):
            flush(ev.ms)
            say(str(take_shot(ev.label)))
            base = ev.ms
        else:
            segment.append(ev)
    flush(base if not segment else segment[-1].ms)
