# 04: Main menu art: labels and preview panels

**What to build:** The main menu shows the designed art from `assets/main-menu` ([spec](../spec.md)), patched in through ticket 03's patcher, in both `MnMaAll` language variants:
- the four labels, RTC 101, Custom Drill, Options and Data (176×30, encoded to IA4), as the frames of the label texture animation that the four selections show;
- the RTC 101 and Custom Drill preview panels (128×104, encoded to I4) as the right-hand panel's images for those two items.

The images are committed with this ticket. The labels must tint and highlight as retail's do: orange fill when not hovered, black with the highlight when hovered, black outline throughout. No material or TEV changes.

Ends with the developer's look check (`ready-for-human` for that sign-off), from the shots. If the art doesn't hold up in game, that's recorded here, and the fallback (labels rendered from an open-licence font, then SIS text over hidden label models) becomes a new ticket.

**Blocked by:** 01, 03

**Status:** ready-for-human

- [x] Shots of the main menu with each of the four items hovered, showing the new labels tinted and highlighted like retail's.
- [x] Shots with the cursor on RTC 101 and on Custom Drill showing their preview panels. Options and Data show their retail panels.
- [x] A comparison shot with Options and Data left as retail labels beside our two, so the developer can choose between redrawn and retail.
- [x] After switching Options' language to Japanese, the main menu shows the same art (shot).
- [ ] The developer has signed off on the look, or a fallback ticket has been filed.

## Comments

- **Frames.** The labels are images 0 ("1-P Mode"), 1 ("VS. Mode"), 3 ("Options") and 4 ("Data") of the texture animation on joint 3 of `MenMainCursor_Top` (frame = selection * 2; image 2, Trophies, is locked and untouched). `.usd` descriptors `0xbffa8`, `0xbffc0`, `0xbfff0`, `0xc0008`; `.dat` `0xbddc8`, `0xbdde0`, `0xbde10`, `0xbde28`. The previews are images 0 and 1 of joint 22's animation on `MenMainConTop_Top` (frame = selection * 50): `.usd` `0x828b4`, `0x828cc`; `.dat` `0x806d4`, `0x806ec`. Options' and Data's panels (images 3, 4) stay retail. All in `main_menu_art()` in `tools/archive_patches.py`. No material or TEV changes.
- **Look.** The art tints orange and goes black with the highlight as retail's labels do, in English and after switching to Japanese. The previews sit on the right panel correctly. Custom Drill's lettering is a little smaller than the others'. Redrawn Options and Data match the two new labels; retail's are visibly larger and rounder (comparison shot `compare-retail-opts-data-*`, made by a temporary local edit that patched only the first two labels). The Japanese menu's "Main Menu" heading is retail's, which ticket 05 covers.
- **Shots** are in the session scratchpad under `shots/04/`: `art-*` (four hovers), `art-ja-*` (Japanese), `compare-retail-opts-data-*`. Scenarios `main-menu-art` and `main-menu-art-japanese`.
- Not signed off: that is the developer's, from the shots.
