# 04: Banner art (developer, with Claude Design)

**What to build:** The disc banner Dolphin's game list and the GameCube menu show for RTC Training ([spec](../spec.md)): a **96×32** image, committed as `assets/banner/banner.png`. The game stores it as RGB5A3 (5 bits per colour, or 4 with 3-bit alpha), so keep it to flat colours and bold shapes that survive that, and readable at 96×32 in a list. Also choose the banner's text: short name (up to 32 characters, e.g. "RTC Training"), maker (up to 32, e.g. the project or your name), and long description (up to 128, two lines). Put the text in `assets/banner/banner.txt`. The same art can head the README (ticket 03) scaled up.

**Blocked by:** None (can start immediately)

**Status:** resolved

**Note:** A placeholder `assets/banner/banner.png` and `banner.txt` are committed (on `iso-release`) so ticket 05 could be built and tested. The placeholder image is the old Training Mode banner ("TRAINING MODE / Made By UnclePunch"), which must not ship. The maintainer will supply the real art and text later by replacing both files; nothing else needs to change (`dev.py check` encodes them, and the README already shows the image).

- [x] `assets/banner/banner.png` is 96×32 and committed.
- [x] `assets/banner/banner.txt` holds short name, maker and description.

## Answer

The maintainer replaced the placeholder with the real art (yellow "RTC 101" on 96×32) and text: short name "RTC Training", maker "RTC Training project", description "Reaction tech chasing practice for Melee." `dev.py check` passes (archive-patches encodes it), and decoding `opening.bnr` from the assembled game folder gives back the art and text. `rtc-training.iso`, built from a Redump-verified disc, boots in Dolphin to the RTC Training main menu, and the new save on the memory card is called "RTC Training". Not checked: the banner in Dolphin's game list.
