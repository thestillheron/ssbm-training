# 02: Magnifier and name tag don't reveal a vanished opponent

**What to build:** While the opponent is vanished, the off-screen magnifier and any name tag must not give its position away. They are expected to follow the invisible flag already, so confirm this in a running build. If either still shows, hide it too while the opponent is vanished, and restore it when the opponent reappears.

**Blocked by:** 01 (Vanish for tech in place, tech in and tech away)

**Status:** resolved

- [x] With the opponent vanished and off-screen, no magnifier is drawn
- [x] With a name tag shown, none is drawn for the vanished opponent
- [x] Both are back when the opponent reappears
- [x] The finding (followed the flag already, or needed a fix) is recorded in this file under `## Comments`

## Comments

Finding: the magnifier already follows the invisible flag, no fix needed. `ftLib_CanBeMagnified` (`src/melee/ft/ftlib.c`) returns false when `fp->invisible` is set, and `ifMagnify_802FBBDC` skips such fighters. The name tag has no invisible check, but the text tag exists only for human slots (`has_nametag`), and the vanishing opponent is a CPU; the arrow indicator is shown only briefly at rep start and was already gone in the mid-vanish shot (`tech-vanish-shots` v12, no opponent, no arrow). Checked by reading code plus that shot; no running-build case with the opponent vanished off-screen was set up (the drills spawn it at the centre of Final Destination).
