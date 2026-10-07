"""Live scenarios: play a `tools.scenario` plan against a running Dolphin.

No Win32 here. Key events go through `tools.pad_live.run_plan` (focus-guarded,
releases held keys on every failure); shot steps call the `take_shot`
callable. Waiting for the game window after `run` also lives here, as does
`follow_movie`: timing the shots of a scenario that Dolphin is playing as a
movie (Dolphin feeds the input; nothing is sent but the shot hotkey).
"""

import time

from tools import movie, pad_live
from tools.pad import PadError
from tools.scenario import Shot, ShotEvent

POLL_S = 0.05  # window lookups are cheap; a short poll pins down when boot starts
FRAME_S = 1001 / 60000  # one NTSC field: Melee's emulated frame
# Movie frame N is drawn at field N + BOOT_OFFSET_FRAMES after power-on.
# Until Melee's pad setup Dolphin polls once per field, not twice, so the
# movie's entries fall 36 (18 frames' worth) behind the fields; measured
# constant on Dolphin 2609 (scenario-movies ticket 04).
BOOT_OFFSET_FRAMES = 18
# But follow_movie's clock starts when the game window appears, which is
# after power-on, and a shot lands a little after its hotkey is sent; together
# about as many fields as the boot offset. Measured live on Dolphin 2609 (a
# shot every 62 frames against a 2-frame press every 8 frames in the main
# menu): a shot sent at window + N fields catches movie frame N to N+4, and
# adding the boot offset to shots made them land about 20 frames late.
SHOT_LAG_FRAMES = 18
MOVIE_END_MARGIN_S = 1.0  # after the movie's last frame, before returning


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


def follow_movie(
    items, take_shot, say=print, check=lambda: None, sleep=time.sleep, clock=time.monotonic
):
    """Follow a scenario movie ([Step | Shot]) that Dolphin started playing
    when its game window appeared (now): call `take_shot(label)` for each
    shot at its frame on the wall clock (input is frame-exact, shot timing is
    approximate), passing the returned path to `say`, then wait until the
    movie has ended. `check()` is called at least every POLL_S while
    waiting and may raise to abort. Returns the movie's length in frames.
    The end is timed as if the window appeared at power-on, the latest the
    movie can end."""
    start = clock()

    def until(fields, margin=0.0):
        t = start + fields * FRAME_S + margin
        while True:
            check()
            left = t - clock()
            if left <= 0:
                return
            sleep(min(left, POLL_S))

    for i, item in enumerate(items):
        if isinstance(item, Shot):
            until(movie.frame_count(items[:i]) + BOOT_OFFSET_FRAMES - SHOT_LAG_FRAMES)
            say(str(take_shot(item.label)))
    frames = movie.frame_count(items)
    until(frames + BOOT_OFFSET_FRAMES, MOVIE_END_MARGIN_S)
    return frames
