"""Scenarios: committed plain-text piloting scripts in tools/scenarios/.

A scenario is `<name>.txt`, one step per line, using the same step grammar as
`dev.py pad` (see tools/pad.py) plus:

  shot <label>     take a screenshot at this point
  include <name>   expand another scenario here, before any input is sent
  # ...            comment (blank lines are ignored too)

Pure logic with no Win32 and no Dolphin. Entry points:

  expand(name, folder)           -> [Step | Shot]   (includes resolved)
  build_plan(items, bindings)    -> [KeyEvent | ShotEvent]  (timed, in order)
  format_plan(events)            -> str
"""

from collections import namedtuple
from pathlib import Path

from . import pad

SCENARIOS_DIR = Path(__file__).resolve().parent / "scenarios"
SUFFIX = ".txt"

Shot = namedtuple("Shot", "label")  # a plan item: screenshot here
ShotEvent = namedtuple("ShotEvent", "ms label")  # a timed Shot


class ScenarioError(Exception):
    pass


def available(folder=SCENARIOS_DIR):
    return sorted(p.stem for p in Path(folder).glob(f"*{SUFFIX}"))


def _chain(chain):
    return " -> ".join(chain)


def _parse_line(name, lineno, line):
    """One non-blank, non-comment line -> ('include', name) | [Step | Shot]."""
    words = line.split()
    where = f"{name}:{lineno}"
    if words[0] in ("shot", "include"):
        if len(words) != 2:
            raise ScenarioError(f"{where}: '{words[0]}' needs exactly one argument: {line}")
        return (words[0], words[1])
    try:
        return [pad.parse_step(w) for w in words]
    except pad.PadError as e:
        raise ScenarioError(f"{where}: {e}")


def expand(name, folder=SCENARIOS_DIR, _chain_so_far=()):
    """Scenario name -> flat [Step | Shot] with every include expanded in
    place. Raises ScenarioError for an unknown scenario, a missing include, a
    cycle (all naming the include chain) or a bad line (naming name:line)."""
    chain = _chain_so_far + (name,)
    if name in _chain_so_far:
        raise ScenarioError(f"include cycle: {_chain(chain)}")
    path = Path(folder) / f"{name}{SUFFIX}"
    if not path.is_file():
        if _chain_so_far:
            raise ScenarioError(f"missing include: {_chain(chain)} ('{name}' not found)")
        names = available(folder)
        raise ScenarioError(
            f"unknown scenario '{name}'. Available: " + (", ".join(names) or "(none)")
        )
    items = []
    for lineno, raw in enumerate(path.read_text().splitlines(), 1):
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        parsed = _parse_line(name, lineno, line)
        if isinstance(parsed, tuple):
            kind, arg = parsed
            if kind == "shot":
                items.append(Shot(arg))
            else:
                items.extend(expand(arg, folder, chain))
        else:
            items.extend(parsed)
    return items


def build_plan(items, bindings):
    """[Step | Shot] -> ordered [KeyEvent | ShotEvent]. Steps run back to back
    (as in pad.build_plan); a shot sits at the time the steps before it end."""
    events = []
    frame = 0
    for item in items:
        offset = pad.frames_to_ms(frame)
        if isinstance(item, Shot):
            events.append(ShotEvent(offset, item.label))
            continue
        for e in pad.build_plan([item], bindings):
            events.append(e._replace(ms=e.ms + offset))
        frame += item.frames
    return events


def format_plan(events):
    lines = []
    for e in events:
        if isinstance(e, ShotEvent):
            lines.append(f"{e.ms:>6} ms  shot  {e.label}")
        else:
            lines.append(pad.format_plan([e]))
    return "\n".join(lines)
