#ifdef TRAINING_BUILD

#include <melee/db/db.h>
#include <sysdolphin/baselib/sislib.h>

/* Position (canvas units) and size of the marker. Tune by eye with `run`. */
#define MARKER_X 480.0F
#define MARKER_Y 85.0F
#define MARKER_SCALE_X 1.0F
#define MARKER_SCALE_Y 0.8F

/* "- T" as the full-width Shift-JIS the game's SIS font expects. */
static char marker_str[] = "\x81\x7C\x81\x40\x82\x73";

/// Draws the permanent "- T" marker next to the "Melee" subtitle on the title
/// screen. Called from gm_Scene_Title_OnEnter.
void training_title_marker(void)
{
    HSD_Text* text;
    int idx;

    /* Debug builds already created this text canvas for the build timestamp. */
    if (DbLevel < DbLKind_NoDebugRom) {
        HSD_SisLib_803A611C(0, NULL, 9, 0xD, 0, 0xE, 0, 0x13);
    }
    text = HSD_SisLib_803A6754(0, 0);
    idx = HSD_SisLib_803A6B98(text, MARKER_X, MARKER_Y, "%s", marker_str);
    text->default_kerning = 1;
    HSD_SisLib_803A7548(text, idx, MARKER_SCALE_X, MARKER_SCALE_Y);
}

#endif
