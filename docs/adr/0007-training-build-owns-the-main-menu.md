# The training build owns the main menu

Amends [ADR-0005](0005-drills-are-event-match-events.md), which kept every vanilla mode reachable and turned down a main-menu entry of our own because of its label art.

The training build's main menu has four items: [RTC 101](../glossary.md#glossary_rtc_101) (the event list), Custom Drill (the [custom drill](../glossary.md#glossary_custom_drill)'s settings menu), Options and Data. 1P Mode, VS Mode and Trophies are gone, so Training Mode, VS and every other vanilla mode can't be reached. Players load the training build to train, and keep a vanilla disc for playing Melee. The new labels replace the main menu's own label textures, so the menu looks designed rather than patched over: artwork made for them first, labels rendered from an open-licence font if that doesn't work out. That makes the training build patch the disc's archives as well as its code: the build writes patched copies into the assembled game folder, as it does `boot.bin`, and never touches the extracted disc.

## Considered Options

- **Keep 1P Mode → Events (ADR-0005):** no art to make, but the way to the curriculum runs through vanilla menus that say "Event Match", and players can wander into modes the build isn't for.
- **Draw our own text over the retail label models:** no archive patching, but our text font doesn't match the retail lettering. This is the last resort if neither the artwork nor the font labels look right.
- **Our own main menu scene:** the most freedom, but Options and Data would still have to be reached as retail menus, for more work than restyling the retail one.

## Consequences

- Vanilla Training Mode is no longer something we keep working, and its scenarios go.
- The label artwork (or, failing that, an open-licence font) is part of the build.
- The art stays in retail's texture formats and sizes, tinted by the menu's own materials, so highlighting works as it does for retail labels.
- Both language variants of each patched archive (`.usd` English, `.dat` Japanese) get the same English art and text, so Options' language switch can't bring the retail menu back.
