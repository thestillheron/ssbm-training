#ifdef TRAINING_BUILD

#include "text.h"

#include <sysdolphin/baselib/sislib.h>
#include <sysdolphin/baselib/sislib_font.h>

/* Arguments of HSD_SisLib_803A611C, which creates the canvas training text is
 * drawn on. Copied from a call the game makes for Training's own text; the
 * meaning of each is not known, so they are named by position. */
#define TEXT_CANVAS_ARG2 9
#define TEXT_CANVAS_ARG3 0xD
#define TEXT_CANVAS_ARG4 0
#define TEXT_CANVAS_ARG5 0xE
#define TEXT_CANVAS_ARG6 0
#define TEXT_CANVAS_ID 0xB

HSD_Text* training_text_create(void)
{
    int canvas = HSD_SisLib_803A611C(0, NULL, TEXT_CANVAS_ARG2,
                                     TEXT_CANVAS_ARG3, TEXT_CANVAS_ARG4,
                                     TEXT_CANVAS_ARG5, TEXT_CANVAS_ARG6,
                                     TEXT_CANVAS_ID);
    HSD_Text* text = HSD_SisLib_803A6754(0, canvas);

    text->default_kerning = 1;
    return text;
}

HSD_Text* training_text_create_on(int canvas)
{
    HSD_Text* text = HSD_SisLib_803A6754(0, canvas);

    text->default_kerning = 1;
    return text;
}

void training_text_place(HSD_Text* text, float x, float y, float z,
                         float size)
{
    text->pos_x = x;
    text->pos_y = y;
    text->pos_z = z;
    text->font_size.x = size;
    text->font_size.y = size;
}

void training_text_set_default_colour(HSD_Text* text, GXColor colour)
{
    text->text_color = colour;
}

void training_text_free(HSD_Text** text)
{
    if (*text != NULL) {
        HSD_SisLib_803A5CC4(*text);
        *text = NULL;
    }
}

/// The SIS font's own character table, lbl_8040C8C0, lists the full-width
/// form of every printable ASCII character in ASCII order from the space,
/// with the backquote left out. So the full-width form of `c` is entry
/// `c - ' '`, one less after the backquote.
static const SjisChar* sis_char(char c)
{
    if (c > ' ' && c <= '~' && c != '`') {
        return &lbl_8040C8C0[c - ' ' - (c > '`' ? 1 : 0)];
    }
    return &lbl_8040C8C0[0]; /* the space */
}

void training_text_to_sis(char* sis, const char* ascii)
{
    int n;

    for (n = 0; *ascii != 0 && n < TRAINING_TEXT_MAX_CHARS; ascii++, n++) {
        const SjisChar* ch = sis_char(*ascii);

        *sis++ = (char) ch->lead;
        *sis++ = (char) ch->trail;
    }
    *sis = 0;
}

int training_text_add(HSD_Text* text, float x, float y, float scale_x,
                      float scale_y, const char* ascii)
{
    char sis[TRAINING_TEXT_MAX_CHARS * 2 + 1];
    int line;

    training_text_to_sis(sis, ascii);
    line = HSD_SisLib_803A6B98(text, x, y, "%s", sis);
    HSD_SisLib_803A7548(text, line, scale_x, scale_y);
    return line;
}

int training_text_add_line(HSD_Text* text, float y, const char* ascii)
{
    char sis[TRAINING_TEXT_MAX_CHARS * 2 + 1];

    training_text_to_sis(sis, ascii);
    return HSD_SisLib_803A6B98(text, 0.0F, y, "%s", sis);
}

void training_text_set(HSD_Text* text, int line, const char* ascii)
{
    char sis[TRAINING_TEXT_MAX_CHARS * 2 + 1];

    training_text_to_sis(sis, ascii);
    HSD_SisLib_803A70A0(text, line, "%s", sis);
}

void training_text_set_colour(HSD_Text* text, int line, GXColor colour)
{
    HSD_SisLib_803A74F0(text, line, &colour);
}

/* HSD_Text alignment that centres each line on its x. */
#define TEXT_ALIGN_CENTRE 1

void training_text_centre(HSD_Text* text)
{
    text->default_alignment = TEXT_ALIGN_CENTRE;
}

void training_text_set_visible(HSD_Text* text, bool visible)
{
    text->hidden = !visible;
}

#endif
