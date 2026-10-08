#ifdef TRAINING_BUILD

#include "score.h"

#include <sysdolphin/baselib/sislib.h>

/* Position (canvas units) and size of the score text. Tune by eye with `run`.
 * Line one is successes out of counted reps, line two the success rate. */
#define SCORE_X 30.0F
#define SCORE_Y 300.0F
#define SCORE_LINE_HEIGHT 28.0F
#define SCORE_SCALE_X 1.0F
#define SCORE_SCALE_Y 0.8F

/* The game's SIS font takes full-width Shift-JIS: digits are 0x824F + digit,
 * the slash is 0x815E and the percent sign is 0x8193. */
#define SJIS_DIGIT_HI 0x82
#define SJIS_DIGIT_LO 0x4F
#define SJIS_SLASH_HI 0x81
#define SJIS_SLASH_LO 0x5E
#define SJIS_PERCENT_HI 0x81
#define SJIS_PERCENT_LO 0x93

/* An update cannot exceed the text first drawn, so each line starts as a
 * placeholder of its longest form: "0000/0000" and "000%". */
static char count_placeholder[] = "\x82\x4F\x82\x4F\x82\x4F\x82\x4F\x81\x5E\x82"
                                  "\x4F\x82\x4F\x82\x4F\x82\x4F";
static char rate_placeholder[] = "\x82\x4F\x82\x4F\x82\x4F\x81\x93";

/* Arguments of HSD_SisLib_803A611C, which creates the canvas training text is
 * drawn on. Copied from a call the game makes for Training's own text; the
 * meaning of each is not known, so they are named by position. */
#define TEXT_CANVAS_ARG2 9
#define TEXT_CANVAS_ARG3 0xD
#define TEXT_CANVAS_ARG4 0
#define TEXT_CANVAS_ARG5 0xE
#define TEXT_CANVAS_ARG6 0
#define TEXT_CANVAS_ID 0xB

static int successes;
static int counted;
static HSD_Text* score_text;
static int line_count_idx;
static int line_rate_idx;

/// Success rate in whole percent. Zero before the first counted rep.
static int success_percent(void)
{
    if (counted == 0) {
        return 0;
    }
    return successes * 100 / counted;
}

static char* put_char(char* dst, int hi, int lo)
{
    *dst++ = (char) hi;
    *dst++ = (char) lo;
    return dst;
}

/// Writes `value` (0 to 9999) as full-width digits.
static char* put_number(char* dst, int value)
{
    int started = 0;
    int div;

    for (div = 1000; div > 0; div /= 10) {
        int digit = value / div % 10;

        if (digit != 0 || started || div == 1) {
            dst = put_char(dst, SJIS_DIGIT_HI, SJIS_DIGIT_LO + digit);
            started = 1;
        }
    }
    return dst;
}

static void redraw(void)
{
    char count_str[24];
    char rate_str[16];
    char* end;

    if (score_text == NULL) {
        return;
    }
    end = put_number(count_str, successes);
    end = put_char(end, SJIS_SLASH_HI, SJIS_SLASH_LO);
    end = put_number(end, counted);
    *end = 0;
    end = put_number(rate_str, success_percent());
    end = put_char(end, SJIS_PERCENT_HI, SJIS_PERCENT_LO);
    *end = 0;
    HSD_SisLib_803A70A0(score_text, line_count_idx, "%s", count_str);
    HSD_SisLib_803A70A0(score_text, line_rate_idx, "%s", rate_str);
}

/// Creates an empty text object on its own canvas, ready for lines to be added.
/// Shared with the "early" notice. Call once the scene has settled: text made
/// during the scene's own setup is not drawn.
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

/// Zeroes the score. Called when Training Mode is entered.
void training_score_init(void)
{
    successes = 0;
    counted = 0;
    score_text = NULL;
}

/// Creates the score text, once the scene has settled (text made during the
/// scene's own setup is not drawn).
void training_score_show(void)
{
    score_text = training_text_create();
    line_count_idx = HSD_SisLib_803A6B98(score_text, SCORE_X, SCORE_Y, "%s",
                                         count_placeholder);
    line_rate_idx =
        HSD_SisLib_803A6B98(score_text, SCORE_X, SCORE_Y + SCORE_LINE_HEIGHT,
                            "%s", rate_placeholder);
    HSD_SisLib_803A7548(score_text, line_count_idx, SCORE_SCALE_X,
                        SCORE_SCALE_Y);
    HSD_SisLib_803A7548(score_text, line_rate_idx, SCORE_SCALE_X,
                        SCORE_SCALE_Y);
    redraw();
}

/// Records a counted rep that ended as a success or a failure. Voids are not
/// recorded.
void training_score_record(bool success)
{
    counted++;
    if (success) {
        successes++;
    }
    redraw();
}

#endif
