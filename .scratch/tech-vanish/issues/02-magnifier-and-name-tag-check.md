# 02: Magnifier and name tag don't reveal a vanished opponent

**What to build:** While the opponent is vanished, the off-screen magnifier and any name tag must not give its position away. They are expected to follow the invisible flag already, so confirm this in a running build. If either still shows, hide it too while the opponent is vanished, and restore it when the opponent reappears.

**Blocked by:** 01 (Vanish for tech in place, tech in and tech away)

**Status:** ready-for-agent

- [ ] With the opponent vanished and off-screen, no magnifier is drawn
- [ ] With a name tag shown, none is drawn for the vanished opponent
- [ ] Both are back when the opponent reappears
- [ ] The finding (followed the flag already, or needed a fix) is recorded in this file under `## Comments`
