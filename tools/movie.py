"""Movies: a scenario compiled into a Dolphin input movie (DTM).

Pure logic with no Win32 and no Dolphin, like tools/scenario.py and
tools/pad.py. Entry point:

  build(items) -> bytes   ([Step | Shot] from scenario.expand -> DTM file)

A DTM file is a 256-byte header followed by one 8-byte controller entry per
input poll of GameCube port 1 (Dolphin's Movie.h; little-endian, packed).
Melee polls the pad twice per emulated frame, so each scenario frame is
written as ENTRIES_PER_FRAME identical entries: a hold of N frames and
`wait:N` both last exactly N emulated frames. Shots write nothing.

The header is a power-on movie (no savestate) for GALE01 with one standard
controller on port 1 that keeps the memory card in slot A as-is (no clear
save): scenarios rely on its everything-unlocked save.

Known single-machine assumption: the header's other fields (video backend,
CPU core and the other emulation settings, Dolphin revision, RTC start,
country code) are copied from a reference movie that the developer's own
Dolphin (release 2609) recorded, with only fast disc speed changed so loads
are short. Playback applies them, so the scenario frame numbers were tuned
under these settings. A different Dolphin version or machine may need its
own reference movie and retuned scenarios.
"""

import struct

from . import pad

ENTRIES_PER_FRAME = 2  # Melee polls the pad twice per emulated frame
GAME_ID = b"GALE01"
HEADER_SIZE = 256
NEUTRAL_AXIS = 128
FULL = 255

# Entry byte 0 / byte 1 bits (Movie.h ControllerState, LSB first).
_BUTTON_BITS = {
    "start": (0, 0x01),
    "a": (0, 0x02),
    "b": (0, 0x04),
    "x": (0, 0x08),
    "y": (0, 0x10),
    "z": (0, 0x20),
    "dpad-up": (0, 0x40),
    "dpad-down": (0, 0x80),
    "dpad-left": (1, 0x01),
    "dpad-right": (1, 0x02),
    "l": (1, 0x04),
    "r": (1, 0x08),
}
_IS_CONNECTED = 0x40  # byte 1 bit 6: without it the pad reads as unplugged
_TRIGGER_BYTE = {"l": 2, "r": 3}
# Stick direction -> (entry byte, value). Y is higher-is-up.
_AXES = {}
for _prefix, (_x, _y) in {"stick": (4, 5), "cstick": (6, 7)}.items():
    _AXES[f"{_prefix}-left"] = (_x, 0)
    _AXES[f"{_prefix}-right"] = (_x, FULL)
    _AXES[f"{_prefix}-down"] = (_y, 0)
    _AXES[f"{_prefix}-up"] = (_y, FULL)

NEUTRAL = bytes([0, _IS_CONNECTED, 0, 0] + [NEUTRAL_AXIS] * 4)


def pad_state(controls):
    """Controls held together -> one 8-byte entry. Controls are already
    validated by pad.parse_step (known, no opposing directions)."""
    e = bytearray(NEUTRAL)
    for c in controls:
        if c in _BUTTON_BITS:
            byte, bit = _BUTTON_BITS[c]
            e[byte] |= bit
        if c in _TRIGGER_BYTE:
            e[_TRIGGER_BYTE[c]] = FULL
        if c in _AXES:
            byte, value = _AXES[c]
            e[byte] = value
    return bytes(e)


def header(entries):
    h = bytearray(HEADER_SIZE)
    h[0x00:0x04] = b"DTM\x1a"
    h[0x04:0x0A] = GAME_ID
    h[0x0A] = 0  # GameCube, not Wii
    h[0x0B] = 0x01  # controllers: GC port 1 only
    h[0x0C] = 0  # power-on, not from a savestate
    struct.pack_into("<QQQ", h, 0x0D, entries // ENTRIES_PER_FRAME, entries, 0)
    h[0x51:0x54] = b"D3D"  # video backend
    # md5 (0x71..0x80) stays zero: Dolphin then skips the check; the booted
    # main.dol changes every build anyway.
    struct.pack_into("<Q", h, 0x81, 1791322237)  # recording start = fixed RTC
    # Emulation settings. Playback applies these (not the developer's
    # Dolphin.ini), so they are part of what makes a run frame-exact.
    h[0x89] = 1  # bSaveConfig: the settings below are recorded
    h[0x8A] = 1  # bSkipIdle
    h[0x8B] = 0  # bDualCore
    h[0x8C] = 0  # bProgressive
    h[0x8D] = 1  # bDSPHLE
    # bFastDiscSpeed: the one setting not as recorded. With emulated disc
    # speed every load (boot, main menu, character select, stage) takes
    # seconds, and boot-to-training could not reach Training within its 20 s
    # budget. Fast disc only shortens loads; runs stay frame-exact.
    h[0x8E] = 1
    h[0x8F] = 1  # CPUCore: JIT64
    h[0x90] = 0  # bEFBAccessEnable
    h[0x91] = 1  # bEFBCopyEnable
    h[0x92] = 1  # bSkipEFBCopyToRam
    h[0x93] = 0  # bEFBCopyCacheEnable
    h[0x94] = 0  # bEFBEmulateFormatChanges
    h[0x95] = 0  # bImmediateXFB
    h[0x96] = 1  # bSkipXFBCopyToRam
    h[0x97] = 0x01  # memcards: slot A present
    h[0x98] = 0  # bClearSave: use the existing card
    h[0x9C] = 1  # bPAL60
    h[0x9F] = 1  # bFollowBranch
    h[0xA0] = 1  # bUseFMA
    h[0xA2] = 1  # bWidescreen
    h[0xA3] = 108  # countryCode
    # revision: the git revision of the developer's Dolphin (release 2609).
    h[0xD1:0xE5] = bytes.fromhex("f84df02055ab9610feec48e65648cac5a3c098fa")
    # tickCount: playback of a power-on movie ends once emulated ticks pass
    # it, so make it unreachable and let the entries alone decide the end.
    struct.pack_into("<Q", h, 0xED, 0x7FFFFFFFFFFFFFFF)
    return bytes(h)


def frame_count(items):
    return sum(i.frames for i in items if isinstance(i, pad.Step))


def build(items):
    """[Step | Shot] (includes expanded) -> DTM bytes."""
    body = bytearray()
    for item in items:
        if isinstance(item, pad.Step):
            body += pad_state(item.controls) * (item.frames * ENTRIES_PER_FRAME)
    return header(len(body) // len(NEUTRAL)) + bytes(body)
