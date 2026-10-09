#ifndef TRAINING_TEXT_H
#define TRAINING_TEXT_H

#ifdef TRAINING_BUILD

#include <sysdolphin/baselib/sislib.h>

/* Training on-screen text (text.c). Every line is written as plain ASCII and
 * converted here to the full-width Shift-JIS the game's SIS font expects. */

/* Longest line, in characters. The text library formats each line into 128
 * bytes, and a full-width character takes two. Longer lines are cut. */
#define TRAINING_TEXT_MAX_CHARS 63

/* Creates an empty text object on its own canvas, ready for lines. Call once
 * the scene has settled: text made during the scene's own setup is not
 * drawn. */
HSD_Text* training_text_create(void);

/* Writes `ascii` into `sis` (room for TRAINING_TEXT_MAX_CHARS * 2 + 1 bytes)
 * as full-width Shift-JIS: every printable ASCII character except the
 * backquote; anything else becomes a space. */
void training_text_to_sis(char* sis, const char* ascii);

/* Adds a line of ASCII text at (x, y), scaled, and returns its index. */
int training_text_add(HSD_Text* text, float x, float y, float scale_x,
                      float scale_y, const char* ascii);

/* Replaces the text of line `line`. The new text may be longer than the
 * old. */
void training_text_set(HSD_Text* text, int line, const char* ascii);

/* Colours line `line`. A line is added in the text object's default colour
 * (white). */
void training_text_set_colour(HSD_Text* text, int line, GXColor colour);

/* Centres every line added from now on on its x, instead of starting it
 * there. */
void training_text_centre(HSD_Text* text);

/* Shows or hides the whole text object. */
void training_text_set_visible(HSD_Text* text, bool visible);

#endif

#endif
