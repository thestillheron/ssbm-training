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

`read_reps` is the reusable reader: anything that has a marker (`run`'s
launch, a scenario's start) passes it as `since`.
"""

from pathlib import Path

PREFIX = "[rep]"

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
