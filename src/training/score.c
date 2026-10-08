#ifdef TRAINING_BUILD

#include "score.h"

#include "drill.h"

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
/* The same counts per tech option, for the end-of-drill summary. */
static int option_successes[TechOption_Count];
static int option_counted[TechOption_Count];
static HSD_Text* score_text;
static int line_count_idx;

/* The drill summary: a few lines of text near the screen centre. Each line is
 * converted to the full-width Shift-JIS the SIS font expects. */
#define SUMMARY_X 200.0F
#define SUMMARY_Y 90.0F
#define SUMMARY_LINE_HEIGHT 30.0F
#define SUMMARY_SCALE_X 1.0F
#define SUMMARY_SCALE_Y 0.9F
#define SUMMARY_MAX_LINES 10
#define SUMMARY_LINE_CHARS 40

static HSD_Text* summary_text;
/* The text keeps the pointers it is given, so each line has its own buffer. */
static char summary_lines[SUMMARY_MAX_LINES][SUMMARY_LINE_CHARS * 2 + 1];
static int summary_line_count;
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
    summary_text = NULL;
    score_text = NULL;
    training_score_clear();
}

/// Zeroes the counts and redraws the score. Restarting a drill does this.
void training_score_clear(void)
{
    int i;

    successes = 0;
    counted = 0;
    for (i = 0; i < TechOption_Count; i++) {
        option_successes[i] = 0;
        option_counted[i] = 0;
    }
    redraw();
}

int training_score_successes(void)
{
    return successes;
}

int training_score_counted(void)
{
    return counted;
}

/// True when the successes so far reach the drill's pass mark.
bool training_score_passed(const Drill* drill)
{
    return successes >= drill->pass_mark;
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
void training_score_record(TechOption option, bool success)
{
    counted++;
    option_counted[option]++;
    if (success) {
        successes++;
        option_successes[option]++;
    }
    redraw();
}

/// Writes the ASCII `src` as full-width Shift-JIS into `dst`: letters, digits,
/// and a few punctuation marks; anything else becomes a space.
static void put_text(char* dst, const char* src)
{
    int n;

    for (n = 0; *src != 0 && n < SUMMARY_LINE_CHARS; src++, n++) {
        char c = *src;

        if (c >= '0' && c <= '9') {
            dst = put_char(dst, 0x82, 0x4F + (c - '0'));
        } else if (c >= 'A' && c <= 'Z') {
            dst = put_char(dst, 0x82, 0x60 + (c - 'A'));
        } else if (c >= 'a' && c <= 'z') {
            dst = put_char(dst, 0x82, 0x81 + (c - 'a'));
        } else if (c == '/') {
            dst = put_char(dst, SJIS_SLASH_HI, SJIS_SLASH_LO);
        } else if (c == '%') {
            dst = put_char(dst, SJIS_PERCENT_HI, SJIS_PERCENT_LO);
        } else if (c == ':') {
            dst = put_char(dst, 0x81, 0x46);
        } else if (c == '-') {
            dst = put_char(dst, 0x81, 0x7C);
        } else if (c == '.') {
            dst = put_char(dst, 0x81, 0x44);
        } else {
            dst = put_char(dst, 0x81, 0x40);
        }
    }
    *dst = 0;
}

static void add_summary_line(const char* ascii)
{
    int idx;

    if (summary_line_count >= SUMMARY_MAX_LINES) {
        return;
    }
    put_text(summary_lines[summary_line_count], ascii);
    idx = HSD_SisLib_803A6B98(
        summary_text, SUMMARY_X,
        SUMMARY_Y + SUMMARY_LINE_HEIGHT * summary_line_count, "%s",
        summary_lines[summary_line_count]);
    HSD_SisLib_803A7548(summary_text, idx, SUMMARY_SCALE_X, SUMMARY_SCALE_Y);
    summary_line_count++;
}

/// Formats "<label> <a>/<b>" into `dst` (two digits each).
static void format_fraction(char* dst, const char* label, int a, int b)
{
    char* p = dst;

    while (*label != 0) {
        *p++ = *label++;
    }
    *p++ = ' ';
    *p++ = '0' + a / 10 % 10;
    *p++ = '0' + a % 10;
    *p++ = '/';
    *p++ = '0' + b / 10 % 10;
    *p++ = '0' + b % 10;
    *p = 0;
}

/// Draws the end-of-drill summary: score, pass mark, pass or fail, and the
/// successes per tech option in the drill's pool.
void training_summary_show(const Drill* drill)
{
    static const char* const option_labels[TechOption_Count] = {
        "In place", "In", "Away", "Miss"
    };
    char line[SUMMARY_LINE_CHARS + 1];
    int i;

    training_summary_hide();
    summary_text = training_text_create();
    summary_line_count = 0;
    add_summary_line(drill->name);
    format_fraction(line, "Score", successes, counted);
    add_summary_line(line);
    format_fraction(line, "Pass mark", drill->pass_mark,
                    drill->length_kind == DrillLength_Time ? counted
                                                           : drill->length);
    add_summary_line(line);
    add_summary_line(training_score_passed(drill) ? "PASS" : "FAIL");
    for (i = 0; i < TechOption_Count; i++) {
        if (drill->option_pool & TECH_OPTION_BIT(i)) {
            format_fraction(line, option_labels[i], option_successes[i],
                            option_counted[i]);
            add_summary_line(line);
        }
    }
    add_summary_line("Press A to restart");
}

/// Hides the summary, if shown.
void training_summary_hide(void)
{
    if (summary_text != NULL) {
        summary_text->hidden = true;
        summary_text = NULL;
    }
}

#endif
