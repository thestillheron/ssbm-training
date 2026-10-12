#ifndef TRAINING_TEXT_H
#define TRAINING_TEXT_H

#ifdef TRAINING_BUILD

#include <sysdolphin/baselib/gobjgxlink.h>
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

/* Creates a canvas of its own and returns it, for training_text_create_on:
 * several objects (rectangles and text) that share one canvas, drawn in the
 * order they were made. */
int training_text_canvas_create(void);

/* Creates a filled rectangle at (x, y), `w` by `h` canvas units, on a canvas
 * (training_text_canvas_create's, say). Draws over what was made before it on
 * that canvas. Free it with training_text_free. */
HSD_Text* training_text_rect_on(int canvas, float x, float y, float w, float h,
                                GXColor colour);

/* Creates an empty text object on one of the game's existing text canvases
 * (the retail menus' canvas 1, for example) instead of a canvas of its own.
 * Place it with training_text_place before adding lines. */
HSD_Text* training_text_create_on(int canvas);

/* Puts the text object at (x, y, z) in its canvas's units with `size` as its
 * font size in both directions. Lines added with training_text_add_line are
 * placed relative to it. */
void training_text_place(HSD_Text* text, float x, float y, float z,
                         float size);

/* Colours every line added from now on. */
void training_text_set_default_colour(HSD_Text* text, GXColor colour);

/* Frees a text object made by training_text_create_on and clears the
 * pointer; a NULL object is left alone. */
void training_text_free(HSD_Text** text);

/* Writes `ascii` into `sis` (room for TRAINING_TEXT_MAX_CHARS * 2 + 1 bytes)
 * as full-width Shift-JIS: every printable ASCII character except the
 * backquote; anything else becomes a space. */
void training_text_to_sis(char* sis, const char* ascii);

/* Adds a line of ASCII text at (x, y), scaled, and returns its index. */
int training_text_add(HSD_Text* text, float x, float y, float scale_x,
                      float scale_y, const char* ascii);

/* Adds an object to a canvas made by training_text_canvas_create that calls
 * `draw` (drawing in the canvas's units, y down, through GX calls) after the
 * text objects made before it and before those made after. Free it with
 * HSD_GObjFree. Costs no text heap, so it draws what a text object each
 * could not: shapes. */
HSD_GObj* training_text_canvas_add_draw(GObj_RenderFunc draw);

/* Adds a line of ASCII text `y` below the text object's own position, at its
 * font size, and returns its index. */
int training_text_add_line(HSD_Text* text, float y, const char* ascii);

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

/* A dark panel with a light border to put text on, so it reads against
 * whatever is behind it. It is two text objects with no lines, each drawing
 * its own background rectangle (the text library's bg_color), the border one
 * made first so it is drawn under the fill. Make the panel before the text
 * it sits behind, so it is drawn first. */
typedef struct TrainingPanel {
    HSD_Text* border;
    HSD_Text* fill;
} TrainingPanel;

/* Makes the panel covering (x, y) to (x + w, y + h) in canvas units, border
 * included, replacing any the panel already had. */
void training_panel_show(TrainingPanel* panel, float x, float y, float w,
                         float h);

/* Moves and resizes a panel already made, keeping its draw order. */
void training_panel_resize(TrainingPanel* panel, float x, float y, float w,
                           float h);

/* Shows or hides the panel; a panel never shown is left alone. */
void training_panel_set_visible(TrainingPanel* panel, bool visible);

/* Frees the panel's text objects and clears it. */
void training_panel_free(TrainingPanel* panel);

#endif

#endif
