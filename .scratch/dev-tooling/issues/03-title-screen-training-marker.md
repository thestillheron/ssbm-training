# 03: "- T" marker on the title screen

Spec: `../spec.md`

**What to build:** When the developer boots the **training build**, the title screen shows "- T" next to the "Melee" subtitle. The matching build never shows it. The marker is permanent: it's how you tell at a glance which build Dolphin has booted.

The "Melee" subtitle is part of the 3D logo model in the title screen's data archive, not text, so we don't edit it. Instead, draw "- T" in the game font using the same text-drawing API the title screen already uses for the debug build timestamp. The drawing code lives in the training source area. The title screen's upstream code gets a single hook call, guarded by the compile-time training switch. This is the first such hook, and it sets the pattern for future ones. Tune the position and scale by eye with `run`.

**Blocked by:** 02

**Status:** resolved

- [ ] `run` shows "- T" beside "Melee" on the title screen. This is a manual check, ideally with a screenshot.
- [ ] The only change in upstream code is one guarded hook line.
- [ ] `build --matching` still produces the byte-identical matching build.
