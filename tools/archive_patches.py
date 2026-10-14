"""The patches the training build makes to the main menu's archives (ADR-0007).

`dev.py` applies `ARCHIVE_PATCHES[name]` to the disc's `files/<name>` when it
assembles the game folder. To change a menu texture or string, add a
replacement to the right lists below (see tools/archive_patcher.py for what a
replacement is). An archive needs the same replacements in both its language
variants (`.usd`, English, and `.dat`, Japanese), so Options' language switch
cannot bring the retail menu back.
"""

from pathlib import Path

from .archive_patcher import Image, StringReplacement, TextureReplacement

ROOT = Path(__file__).resolve().parents[1]
ART = ROOT / "assets" / "main-menu"  # the committed images texture replacements use

# The textures' sizes, which the patcher checks against each descriptor.
LABEL_SIZE = (176, 30)  # IA4
PREVIEW_SIZE = (128, 104)  # I4
HEADING_SIZE = (168, 28)  # I4


def descriptions():
    """SdMenu: the main menu's description box shows SIS entry 0x80 plus the
    item's selection index plus 1. RTC 101 is entry 0x81 and Custom Drill 0x82
    (retail's 1-P Mode and VS. Mode); Options (0x84) and Data (0x85) keep
    retail's text.

    Each text takes two lines, on the layout of retail's two-line entry 0x8D
    (the Japanese menu has none of its own, so both variants copy the English
    one's from the extracted SdMenu.usd). They are longer than the retail
    slots, so they live in the space of descriptions the training build no
    longer shows: entries 0x86-0x8C belong to the 1-P Mode and VS. Mode
    submenus, which it cannot reach."""
    layout = ("SdMenu.usd", 0x8D)
    return [
        StringReplacement(
            0x81,
            "Learn to tech chase on reaction,\none step at a time.",
            template=layout,
            spill=(0x86, 0x88),
        ),
        StringReplacement(
            0x82,
            "Set up a drill of your own:\noptions, stage, layout, vanish.",
            template=layout,
            spill=(0x89, 0x8C),
        ),
    ]


def main_menu_art(label_offsets, preview_offsets):
    """MnMaAll: the label frames and preview panels of the top-level menu.

    The labels are frames 0, 1, 3 and 4 of the cursor model's label texture
    animation (`MenMainCursor_Top`, joint 3): the "1-P Mode", "VS. Mode",
    "Options" and "Data" images, shown for the selections of the same index.
    Frame 2 is "Trophies", which is locked. The previews are images 0 and 1
    of the right-hand panel's animation (`MenMainConTop_Top`, joint 22):
    selection 0 and 1; Options' and Data's panels stay retail's.
    Offsets come from `py ref/main-menu/dump.py <archive> MenMainCursor_Top
    mat` (and `MenMainConTop_Top`) and differ between `.usd` and `.dat`."""
    labels = [
        "label_rtc_101.png",
        "label_custom_drill.png",
        "label_options.png",
        "label_data.png",
    ]
    previews = ["preview_rtc_101.png", "preview_custom_drill.png"]
    return [
        TextureReplacement(offset, ART / name, "IA4", LABEL_SIZE)
        for offset, name in zip(label_offsets, labels)
    ] + [
        TextureReplacement(offset, ART / name, "I4", PREVIEW_SIZE)
        for offset, name in zip(preview_offsets, previews)
    ]


def screen_headings(rtc_101, custom_drill, event_match):
    """MnMaAll: the heading strip (`MenMainPanel_Top`) of the event list and
    the Custom Drill screen, which are the retail Event Match scene
    (cur_menu 7).

    The strip's two joints show frames of their texture animations: joint 84
    (top level) frame 0 "1-P Mode" and joint 85 (second level) frame 1 "Event
    Match" on that scene. Joint 84's frame 0 and 1 ("VS. Mode") are shown by
    1-P Mode's and VS. Mode's own screens only, none of which the training
    build can reach, so they become "RTC 101" and "Custom Drill"; the Custom
    Drill screen picks frame 1 at run time (`training_main_menu_heading`).
    Joint 85's frame 1 is used by that scene alone and becomes blank, leaving
    the second level empty. The joint numbers (84, 85) are counted by
    `nth_matanim_joint` in src/training/main_menu.c, which finds joint 84 at
    run time (keep HEADING_JOINT there in step with this). Options, Data and their submenus use frames 3 and
    4 of joint 84 and other frames of joint 85, which stay retail's.
    Offsets come from `py ref/main-menu/dump.py <archive> MenMainPanel_Top
    mat` and differ between `.usd` and `.dat`."""
    return [
        TextureReplacement(rtc_101, ART / "heading_rtc_101.png", "I4", HEADING_SIZE),
        TextureReplacement(
            custom_drill, ART / "heading_custom_drill.png", "I4", HEADING_SIZE
        ),
        TextureReplacement(
            event_match,
            Image(*HEADING_SIZE, [[(0, 0, 0, 0)] * HEADING_SIZE[0] for _ in range(HEADING_SIZE[1])]),
            "I4",
            HEADING_SIZE,
        ),
    ]


# File name under the disc's files/ -> replacements. A texture's descriptor
# offset differs between the .usd and the .dat.
ARCHIVE_PATCHES = {
    "MnMaAll.usd": main_menu_art(
        (0xBFFA8, 0xBFFC0, 0xBFFF0, 0xC0008), (0x828B4, 0x828CC)
    )
    + screen_headings(0x563F8, 0x56410, 0x564B4),
    "MnMaAll.dat": main_menu_art(
        (0xBDDC8, 0xBDDE0, 0xBDE10, 0xBDE28), (0x806D4, 0x806EC)
    )
    + screen_headings(0x56338, 0x56350, 0x563F4),
    "SdMenu.usd": descriptions(),
    "SdMenu.dat": descriptions(),
}
