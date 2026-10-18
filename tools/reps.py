"""The rep log: `[rep]` lines the training build prints through OSReport,
read back out of Dolphin's log file.

No Dolphin here, so it is testable on fixture files. Entry points:

  log_path(user_dir)                  -> Path   (Dolphin's log file)
  launch_args()                       -> [str]  (`-C key=value` args that make a
                                                 launched Dolphin write the log)
  require_logging(user_dir)                     (RepsError saying how to get a
                                                 log if it isn't there)
  mark(log)                           -> int    (start marker: the log's size now)
  read_reps(log, since=0)             -> [str]  (the `[rep]` lines after offset
                                                 `since`, prefix included, as
                                                 printed by the game)
  layout_lines(lines)                 -> [str]  (only the Print layout lines,
                                                 `[rep] layout: ...`)
  drill_starts(lines)                 -> [dict] (name, height_kind (`list`,
                                                 `range` or `character`), heights)
  drops(lines)                        -> [dict] (height and opponent fighter
                                                 kind of each `drop` line)
  check_drill_starts(lines, names)    -> [str]  (problems with the drills a
                                                 scenario started, see below)

`read_reps` is the reusable reader: anything that has a marker (`run`'s
launch, a scenario's start) passes it as `since`.
"""

import re
from pathlib import Path

PREFIX = "[rep]"
# Print layout's line (not `key=value`; unrelated to `event=layout`).
LAYOUT_PREFIX = PREFIX + " layout: "

# Per-launch Dolphin config overrides (`-C Section.Key=value`) that turn on
# file logging and the OSReport log types.
LOGGER_OVERRIDES = [
    ("Logger.Options.WriteToFile", "True"),
    ("Logger.Logs.OSREPORT", "True"),
]


def launch_args():
    """The Dolphin launch arguments that apply LOGGER_OVERRIDES."""
    return [arg for key, value in LOGGER_OVERRIDES for arg in ("-C", f"{key}={value}")]


class RepsError(Exception):
    pass


def log_path(user_dir):
    return Path(user_dir) / "Logs" / "dolphin.log"


def logger_ini(user_dir):
    return Path(user_dir) / "Config" / "Logger.ini"


def require_logging(user_dir):
    """Raise RepsError unless Dolphin's log file exists.

    `run` turns the logging on for its own launch with LOGGER_OVERRIDES, so
    Logger.ini is not consulted: it may say off while a run-launched Dolphin
    logs fine. A Dolphin that was not launched by `run` writes the file only
    if the developer turned the two settings on in Dolphin itself."""
    log = log_path(user_dir)
    if not log.is_file():
        raise RepsError(
            f"Dolphin has not written its game log ({log}). Launch Dolphin with "
            "`dev.py run` (authoring tools on, the default), which turns the "
            "logging on for that launch. To log from a Dolphin you start yourself, "
            "open View > Show Log Configuration and turn on Write to File and the "
            f"OSREPORT log type (saved in {logger_ini(user_dir)} as WriteToFile "
            "under [Options] and OSREPORT under [Logs]). "
            "Without a log, an empty result would not mean that no reps happened."
        )


def mark(log):
    """The start marker for `log`: its current size, 0 if it doesn't exist."""
    try:
        return Path(log).stat().st_size
    except OSError:
        return 0


def read_reps(log, since=0):
    """The `[rep]` lines written to `log` after byte offset `since`. Dolphin
    prefixes each line with a timestamp and log type; that is dropped. A log
    shorter than `since` was restarted, so it is read from the beginning."""
    data = Path(log).read_bytes()
    if since > len(data):
        since = 0
    out = []
    for line in data[since:].decode("utf-8", errors="replace").splitlines():
        i = line.find(PREFIX)
        if i >= 0:
            out.append(line[i:].rstrip())
    return out


def layout_lines(lines):
    """The Print layout lines among `lines` (as read_reps returns them)."""
    return [line for line in lines if line.startswith(LAYOUT_PREFIX)]


_DRILL_START_FIELDS = re.compile(
    r'event=drill_start name="([^"]*)" stage=\S+ height_kind=(\S+) heights=(\S*)'
)
_DROP = re.compile(r"event=drop .*?\bheight=(-?\d+)(?:.*?\bopponent=(-?\d+))?")


def drill_starts(lines):
    """The parsed `drill_start` lines among `lines`: each drill's name, its
    height kind (`list`, `range` or `character`) and its heights (empty for
    `character`)."""
    out = []
    for line in lines:
        m = _DRILL_START_FIELDS.search(line)
        if m:
            heights = [int(h) for h in m.group(3).split(",") if h]
            out.append({"name": m.group(1), "height_kind": m.group(2), "heights": heights})
    return out


def drops(lines):
    """The parsed `drop` lines among `lines`: the height the opponent dropped
    from and its fighter kind (None on a line from before the field was
    logged)."""
    out = []
    for line in lines:
        m = _DROP.search(line)
        if m:
            opp = m.group(2)
            out.append({"height": int(m.group(1)), "opponent": int(opp) if opp is not None else None})
    return out


# Test drills that are meant to be invalid layouts, and the problem each one
# must report (drill.c's authoring-only test drills).
INTENDED_INVALID = {
    "Test: invalid layout": "no-floor-below-landing",
    "Test: no heights": "bad-drop-heights",
}

_DRILL_START = re.compile(r'event=drill_start name="([^"]*)"')
_INVALID_LAYOUT = re.compile(r'event=invalid_layout drill="([^"]*)" problem=(\S+)')


def check_drill_starts(lines, names):
    """The problems in the `drill_start` lines of a scenario that started every
    drill in `names` (the drill table's): an invalid layout (or a stage no
    drill fits) where none is intended, an intended-invalid test drill that did
    not report its problem, and a drill that never started. Empty when all is
    well."""
    started = []
    invalid = {}
    problems = []
    for line in lines:
        m = _DRILL_START.search(line)
        if m:
            started.append(m.group(1))
        m = _INVALID_LAYOUT.search(line)
        if m:
            invalid[m.group(1)] = m.group(2)
        if "event=no_drill_for_stage" in line:
            problems.append(f"no drill fits the stage: {line}")
    for name in names:
        if name not in started:
            problems.append(f'{name}: never started')
    for name in started:
        want = INTENDED_INVALID.get(name)
        got = invalid.get(name)
        if want is None and got is not None:
            problems.append(f"{name}: invalid layout ({got}), not expected")
        elif want is not None and got != want:
            problems.append(f"{name}: expected invalid layout ({want}), got {got or 'none'}")
    return problems
