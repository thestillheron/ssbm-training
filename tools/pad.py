"""Piloting: the step grammar and the GameCube-control -> keyboard mapping.

Pure logic with no Win32 and no Dolphin, so it works for `dev.py pad
--dry-run` anywhere. Entry points, in the order a caller uses them:

  parse_sequence(text)            -> [Step]            (grammar)
  pad_config_path(user_dir)       -> Path
  load_bindings(pad_config_path)  -> {control: Key}    (fresh read of [GCPad1])
  build_plan(steps, bindings)     -> [KeyEvent]        (timed key down/up)
  format_plan(events)             -> str
"""

import re
from collections import namedtuple
from pathlib import Path

FPS = 60
DEFAULT_HOLD_FRAMES = 3

BUTTONS = ("a", "b", "x", "y", "z", "start", "l", "r")
DIRECTIONS = ("up", "down", "left", "right")
CONTROLS = (
    BUTTONS
    + tuple(f"{p}-{d}" for p in ("stick", "cstick", "dpad") for d in DIRECTIONS)
)

# Control -> the key name used in Dolphin's [GCPad1] section.
DOLPHIN_KEYS = {
    "a": "Buttons/A",
    "b": "Buttons/B",
    "x": "Buttons/X",
    "y": "Buttons/Y",
    "z": "Buttons/Z",
    "start": "Buttons/Start",
    "l": "Triggers/L",
    "r": "Triggers/R",
}
for _d in DIRECTIONS:
    DOLPHIN_KEYS[f"stick-{_d}"] = f"Main Stick/{_d.capitalize()}"
    DOLPHIN_KEYS[f"cstick-{_d}"] = f"C-Stick/{_d.capitalize()}"
    DOLPHIN_KEYS[f"dpad-{_d}"] = f"D-Pad/{_d.capitalize()}"

Step = namedtuple("Step", "text controls frames")  # controls == () for a wait
Key = namedtuple("Key", "name scancode extended")  # Dolphin name, DIK scancode
KeyEvent = namedtuple("KeyEvent", "ms action control key")  # action: down|up


class PadError(Exception):
    pass


# ------------------------------------------------------------- grammar

_STEP_RE = re.compile(r"^([a-z-]+(?:\+[a-z-]+)*)(?::(\d+))?$")
_WAIT_RE = re.compile(r"^wait:(\d+)$")


def parse_step(text):
    m = _WAIT_RE.match(text)
    if m:
        frames = int(m.group(1))
        if frames < 1:
            raise PadError(f"malformed step '{text}': a wait needs at least 1 frame")
        return Step(text, (), frames)
    m = _STEP_RE.match(text)
    if not m or text.split(":")[0] == "wait":
        raise PadError(
            f"malformed step '{text}': expected controls joined by '+' with an "
            "optional ':frames' (e.g. 'stick-down+b:6'), or 'wait:frames'"
        )
    controls = tuple(m.group(1).split("+"))
    for c in controls:
        if c not in CONTROLS:
            raise PadError(
                f"unknown control '{c}' in step '{text}'. Controls: "
                + " ".join(CONTROLS)
            )
    frames = int(m.group(2)) if m.group(2) is not None else DEFAULT_HOLD_FRAMES
    if frames < 1:
        raise PadError(f"malformed step '{text}': a hold needs at least 1 frame")
    return Step(text, controls, frames)


def parse_sequence(text):
    """Whitespace-separated steps -> [Step]. Raises PadError on a bad step."""
    steps = [parse_step(t) for t in text.split()]
    if not steps:
        raise PadError("empty sequence")
    return steps


# ------------------------------------------------------------- bindings

# DirectInput (set 1) scancodes. Extended keys carry an E0 prefix.
_SCANCODES = {
    **dict(zip("1234567890", range(0x02, 0x0C))),
    **dict(zip("QWERTYUIOP", range(0x10, 0x1A))),
    **dict(zip("ASDFGHJKL", range(0x1E, 0x27))),
    **dict(zip("ZXCVBNM", range(0x2C, 0x33))),
    **{f"F{n}": 0x3A + n for n in range(1, 11)},
    "F11": 0x57,
    "F12": 0x58,
    "ESCAPE": 0x01,
    "MINUS": 0x0C,
    "EQUALS": 0x0D,
    "BACK": 0x0E,
    "TAB": 0x0F,
    "LBRACKET": 0x1A,
    "RBRACKET": 0x1B,
    "RETURN": 0x1C,
    "LCONTROL": 0x1D,
    "SEMICOLON": 0x27,
    "APOSTROPHE": 0x28,
    "GRAVE": 0x29,
    "LSHIFT": 0x2A,
    "BACKSLASH": 0x2B,
    "COMMA": 0x33,
    "PERIOD": 0x34,
    "SLASH": 0x35,
    "RSHIFT": 0x36,
    "LALT": 0x38,
    "SPACE": 0x39,
    "CAPITAL": 0x3A,
    "NUMLOCK": 0x45,
    "SCROLL": 0x46,
    "MULTIPLY": 0x37,
    "SUBTRACT": 0x4A,
    "ADD": 0x4E,
    "DECIMAL": 0x53,
    **{
        f"NUMPAD{n}": code
        for n, code in enumerate((0x52, 0x4F, 0x50, 0x51, 0x4B, 0x4C, 0x4D, 0x47, 0x48, 0x49))
    },
}
_EXTENDED = {
    "UP": 0x48,
    "DOWN": 0x50,
    "LEFT": 0x4B,
    "RIGHT": 0x4D,
    "RCONTROL": 0x1D,
    "RALT": 0x38,
    "HOME": 0x47,
    "END": 0x4F,
    "PRIOR": 0x49,
    "NEXT": 0x51,
    "INSERT": 0x52,
    "DELETE": 0x53,
    "DIVIDE": 0x35,
    "NUMPADENTER": 0x1C,
}
# Alternative spellings Dolphin shows (backtick-quoted when they have spaces).
_ALIASES = {
    "ENTER": "RETURN",
    "BACKSPACE": "BACK",
    "ESC": "ESCAPE",
    "SHIFT": "LSHIFT",
    "LEFT SHIFT": "LSHIFT",
    "RIGHT SHIFT": "RSHIFT",
    "CONTROL": "LCONTROL",
    "CTRL": "LCONTROL",
    "LEFT CONTROL": "LCONTROL",
    "RIGHT CONTROL": "RCONTROL",
    "ALT": "LALT",
    "LEFT ALT": "LALT",
    "RIGHT ALT": "RALT",
    "BACKTICK": "GRAVE",
    "UP ARROW": "UP",
    "DOWN ARROW": "DOWN",
    "LEFT ARROW": "LEFT",
    "RIGHT ARROW": "RIGHT",
    "PAGE UP": "PRIOR",
    "PAGE DOWN": "NEXT",
    "SPACEBAR": "SPACE",
    "CAPS LOCK": "CAPITAL",
    "CAPSLOCK": "CAPITAL",
    "NUM LOCK": "NUMLOCK",
}


def key_from_name(name):
    """Dolphin binding name (`X`, `COMMA`, `Left Shift`) -> Key, or None."""
    bare = name.strip().strip("`").strip()
    up = _ALIASES.get(bare.upper(), bare.upper())
    if up in _SCANCODES:
        return Key(bare, _SCANCODES[up], False)
    if up in _EXTENDED:
        return Key(bare, _EXTENDED[up], True)
    return None


def pad_config_path(user_dir):
    return Path(user_dir) / "Config" / "GCPadNew.ini"


def read_section(path, section):
    """{key: value} of one INI section, or None if the section is absent."""
    result = None
    inside = False
    for raw in Path(path).read_text(encoding="utf-8-sig").splitlines():
        line = raw.strip()
        if line.startswith("[") and line.endswith("]"):
            inside = line[1:-1] == section
            if inside:
                result = {}
        elif inside and "=" in line and line[0] not in "#;":
            k, _, v = line.partition("=")
            result[k.strip()] = v.strip()
    return result


class Bindings(dict):
    """{control: Key}, plus `unmappable`: {control: key name} for controls
    bound to a key piloting can't send."""

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.unmappable = {}


def load_bindings(config_path):
    """Fresh read of [GCPad1] -> {control: Key} for every bound control.

    Raises PadError if the file is missing, port 1 has no section, or its
    device is not the keyboard. Unbound controls are absent from the result;
    controls bound to a key we can't send are recorded in the result's
    `unmappable` ({control: key name}); see resolve().
    """
    config_path = Path(config_path)
    if not config_path.is_file():
        raise PadError(
            f"missing Dolphin pad config: {config_path}. Configure GameCube "
            "port 1 as a keyboard controller in Dolphin (Controllers) first, "
            'or set "dolphin_user" in dev.config.json to the right user folder.'
        )
    section = read_section(config_path, "GCPad1")
    if section is None:
        raise PadError(f"no [GCPad1] section in {config_path}")
    device = section.get("Device", "")
    if "keyboard" not in device.lower():
        raise PadError(
            f"GameCube port 1 is bound to '{device or 'no device'}', not the "
            "keyboard. Piloting sends keystrokes, so set port 1 to the "
            "keyboard device (e.g. DInput/0/Keyboard Mouse) in Dolphin."
        )
    bindings = Bindings()
    for control, dolphin_key in DOLPHIN_KEYS.items():
        value = section.get(dolphin_key)
        if value:
            key = key_from_name(value)
            if key:
                bindings[control] = key
            else:
                bindings.unmappable[control] = value.strip().strip("`").strip()
    return bindings


def resolve(control, bindings, section_hint="[GCPad1]"):
    key = bindings.get(control)
    if key is None:
        name = getattr(bindings, "unmappable", {}).get(control)
        if name is not None:
            raise PadError(
                f"control '{control}' is bound to '{name}', which piloting "
                f"can't send; rebind {DOLPHIN_KEYS[control]} in {section_hint} "
                "to a plain keyboard key"
            )
        raise PadError(
            f"unbound control '{control}': no keyboard key for "
            f"{DOLPHIN_KEYS[control]} in {section_hint} of Dolphin's pad config"
        )
    return key


# ------------------------------------------------------------- plan


def frames_to_ms(frames):
    return round(frames * 1000 / FPS)


def build_plan(steps, bindings):
    """Steps -> ordered [KeyEvent]. Steps run back to back; within a step all
    controls go down together and come up together after the hold."""
    events = []
    frame = 0
    for step in steps:
        keys = [(c, resolve(c, bindings)) for c in step.controls]
        for c, k in keys:
            events.append(KeyEvent(frames_to_ms(frame), "down", c, k))
        frame += step.frames
        for c, k in keys:
            events.append(KeyEvent(frames_to_ms(frame), "up", c, k))
    return events


def format_plan(events):
    lines = []
    for e in events:
        code = f"0x{e.key.scancode:02X}"
        lines.append(
            f"{e.ms:>6} ms  {e.action:<4}  {e.control:<11}  "
            f"{e.key.name}{' (extended)' if e.key.extended else ''}  {code}"
        )
    return "\n".join(lines)
