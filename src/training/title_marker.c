#ifdef TRAINING_BUILD

#include "text.h"

#include <melee/db/db.h>
#include <sysdolphin/baselib/sislib.h>

/* Position (canvas units) and size of the marker. Tune by eye with `run`. */
#define MARKER_X 480.0F
#define MARKER_Y 85.0F
#define MARKER_SCALE_X 1.0F
#define MARKER_SCALE_Y 0.8F

/// Draws the permanent "- T" marker next to the "Melee" subtitle on the title
/// screen. Called from gm_Scene_Title_OnEnter.
void training_title_marker(void)
{
    HSD_Text* text;

    /* Debug builds already created this text canvas for the build timestamp. */
    if (DbLevel < DbLKind_NoDebugRom) {
        HSD_SisLib_803A611C(0, NULL, 9, 0xD, 0, 0xE, 0, 0x13);
    }
    text = HSD_SisLib_803A6754(0, 0);
    text->default_kerning = 1;
    training_text_add(text, MARKER_X, MARKER_Y, MARKER_SCALE_X, MARKER_SCALE_Y,
                      "- T");
}

#endif
