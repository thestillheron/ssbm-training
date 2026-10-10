#ifdef TRAINING_BUILD

#include "score.h"

#include "drill.h"
#include "text.h"

#include <sysdolphin/baselib/sislib.h>

#include <stdio.h>
#include <string.h>

/* Position (canvas units) and size of the score text: top left, clear
 * of the rep result above the damage HUD. Tune by
 * eye with `run`.
 * Line one is successes out of counted reps, line two the success rate. */
#define SCORE_X 30.0F
#define SCORE_Y 75.0F
#define SCORE_LINE_HEIGHT 20.0F
#define SCORE_SCALE_X 0.7F
#define SCORE_SCALE_Y 0.6F
/* A character is about 20 canvas units wide at scale 1. */
#define SCORE_CHAR_WIDTH (20.0F * SCORE_SCALE_X)
/* The panel behind the score hugs the text, 5 pixels between the text's ink
 * and the border's inner edge on every side (measured from a screenshot, so
 * the numbers fold in the glyphs' own side room and the border's width). Its
 * width follows the longer of the two lines (redraw). */
#define SCORE_PANEL_LEFT 7.0F
#define SCORE_PANEL_RIGHT 10.0F
#define SCORE_PANEL_TOP 6.5F
#define SCORE_PANEL_HEIGHT 50.5F

static int successes;
static int counted;
/* The same counts per tech the opponent did, with early and late failures,
 * for the end-of-drill summary. */
static TechScore techs[RepTech_Count];
static HSD_Text* score_text;
static TrainingPanel score_panel;
static int line_count_idx;
static int line_rate_idx;

/* The drill summary: the drill's name, then its score and pass mark and the
 * verdict, then a table of the options that came up (one row each, under a
 * header), smaller, so its four columns fit across. Column x positions fit the
 * widest cells: "In place", "10/10 100%", "Early", "Late" (about 11 canvas
 * units a character at the table's scale). Gaps set the blocks apart. Tune by
 * eye with `run`. */
#define SUMMARY_X 265.0F
#define SUMMARY_Y 100.0F
#define SUMMARY_TITLE_SCALE 0.8F
#define SUMMARY_TITLE_HEIGHT 26.0F
#define SUMMARY_TITLE_GAP 18.0F
#define SUMMARY_LINE_HEIGHT 28.0F
#define SUMMARY_SCALE 0.65F
#define SUMMARY_VERDICT_GAP 12.0F
#define SUMMARY_TABLE_GAP 20.0F
#define SUMMARY_TABLE_LINE_HEIGHT 26.0F
#define SUMMARY_TABLE_SCALE 0.55F
#define SUMMARY_COLUMNS 4

static const float summary_column_x[SUMMARY_COLUMNS] = { SUMMARY_X, 365.0F,
                                                          482.0F, 545.0F };

/* The panel behind the summary: it spans the table's columns, and is as
 * tall as the blocks the summary draws. */
#define SUMMARY_PANEL_PAD_X 14.0F
#define SUMMARY_PANEL_PAD_Y 10.0F
/* The last line's glyphs hang below the height its row is given. */
#define SUMMARY_PANEL_BOTTOM_EXTRA 12.0F
#define SUMMARY_PANEL_W 355.0F

static HSD_Text* summary_text;
static TrainingPanel summary_panel;
/* Canvas y of the next summary line. */
static float summary_y;

/// Success rate in whole percent. Zero before the first counted rep.
static int success_percent(void)
{
    if (counted == 0) {
        return 0;
    }
    return successes * 100 / counted;
}

static void redraw(void)
{
    char line[24];
    int width;

    if (score_text == NULL) {
        return;
    }
    sprintf(line, "%d/%d", successes, counted);
    training_text_set(score_text, line_count_idx, line);
    width = (int) strlen(line);
    sprintf(line, "%d%%", success_percent());
    training_text_set(score_text, line_rate_idx, line);
    if ((int) strlen(line) > width) {
        width = (int) strlen(line);
    }
    training_panel_resize(&score_panel, SCORE_X - SCORE_PANEL_LEFT,
                          SCORE_Y + SCORE_PANEL_TOP,
                          width * SCORE_CHAR_WIDTH + SCORE_PANEL_LEFT +
                              SCORE_PANEL_RIGHT,
                          SCORE_PANEL_HEIGHT);
}

/// Zeroes the score. Called when a drill event starts.
void training_score_init(void)
{
    summary_text = NULL;
    summary_panel.border = NULL;
    summary_panel.fill = NULL;
    score_text = NULL;
    score_panel.border = NULL;
    score_panel.fill = NULL;
    training_score_clear();
}

/// Zeroes the counts and redraws the score. Restarting a drill does this.
void training_score_clear(void)
{
    int i;

    successes = 0;
    counted = 0;
    for (i = 0; i < RepTech_Count; i++) {
        techs[i].counted = 0;
        techs[i].successes = 0;
        techs[i].early = 0;
        techs[i].late = 0;
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

/// True when the drill has a pass mark and the successes so far reach it
/// (a drill with no pass mark is never passed).
bool training_score_passed(const Drill* drill)
{
    return training_drill_has_pass_mark(drill) &&
           successes >= drill->pass_mark;
}

/// Creates the score text, once the scene has settled (text made during the
/// scene's own setup is not drawn). Each line starts short and grows as the
/// counts do.
void training_score_show(void)
{
    training_panel_show(&score_panel, SCORE_X - SCORE_PANEL_LEFT,
                        SCORE_Y + SCORE_PANEL_TOP, 0.0F, 0.0F);
    score_text = training_text_create();
    line_count_idx = training_text_add(score_text, SCORE_X, SCORE_Y,
                                       SCORE_SCALE_X, SCORE_SCALE_Y, "0/0");
    line_rate_idx = training_text_add(score_text, SCORE_X,
                                      SCORE_Y + SCORE_LINE_HEIGHT,
                                      SCORE_SCALE_X, SCORE_SCALE_Y, "0%");
    redraw();
}

/// Records a counted rep that ended as a success or a failure, against the
/// tech the opponent did. Voids are not recorded.
void training_score_record(RepTech tech, bool success)
{
    counted++;
    techs[tech].counted++;
    if (success) {
        successes++;
        techs[tech].successes++;
    }
    redraw();
}

/// Counts a counted rep's final timing class against its tech: early and
/// late failures are kept, other classes ignored.
void training_score_record_timing(RepTech tech, RepTimingKind kind)
{
    if (kind == RepTiming_Early) {
        techs[tech].early++;
    } else if (kind == RepTiming_Late) {
        techs[tech].late++;
    }
}

/// The counts for one tech in the current drill run.
TechScore training_score_tech(RepTech tech)
{
    return techs[tech];
}

static void add_summary_line(const char* ascii)
{
    training_text_add(summary_text, SUMMARY_X, summary_y, SUMMARY_SCALE,
                      SUMMARY_SCALE, ascii);
    summary_y += SUMMARY_LINE_HEIGHT;
}

/// Adds one row of the summary's option table, a cell per column.
static void add_summary_row(const char* const cells[SUMMARY_COLUMNS])
{
    int i;

    for (i = 0; i < SUMMARY_COLUMNS; i++) {
        training_text_add(summary_text, summary_column_x[i], summary_y,
                          SUMMARY_TABLE_SCALE, SUMMARY_TABLE_SCALE, cells[i]);
    }
    summary_y += SUMMARY_TABLE_LINE_HEIGHT;
}

/// Formats "<label> <a>/<b>" into `dst` (two digits each).
static void format_fraction(char* dst, const char* label, int a, int b)
{
    sprintf(dst, "%s %02d/%02d", label, a % 100, b % 100);
}

/// Adds the option table: a header, then a row for each tech the opponent
/// did in a counted rep, with its successes out of its reps, its success
/// rate, and its early and late failures. Nothing if no rep was counted.
static void add_option_table(void)
{
    static const char* const header[SUMMARY_COLUMNS] = { "Option", "Success",
                                                         "Early", "Late" };
    char rate[24];
    char early[8];
    char late[8];
    const char* cells[SUMMARY_COLUMNS];
    int i;

    if (counted == 0) {
        return;
    }
    summary_y += SUMMARY_TABLE_GAP;
    add_summary_row(header);
    for (i = 0; i < RepTech_Count; i++) {
        const TechScore* o = &techs[i];

        if (o->counted == 0) {
            continue;
        }
        sprintf(rate, "%d/%d %d%%", o->successes, o->counted,
                o->successes * 100 / o->counted);
        sprintf(early, "%d", o->early);
        sprintf(late, "%d", o->late);
        cells[0] = training_rep_tech_short_label((RepTech) i);
        cells[1] = rate;
        cells[2] = early;
        cells[3] = late;
        add_summary_row(cells);
    }
}

/// The height of the summary's blocks, in canvas units.
static float summary_height(const Drill* drill)
{
    float height = SUMMARY_TITLE_HEIGHT + SUMMARY_TITLE_GAP +
                   SUMMARY_LINE_HEIGHT;
    int rows = 1;
    int i;

    if (training_drill_has_pass_mark(drill)) {
        height += 2.0F * SUMMARY_LINE_HEIGHT + SUMMARY_VERDICT_GAP;
    }
    if (counted != 0) {
        for (i = 0; i < RepTech_Count; i++) {
            if (techs[i].counted != 0) {
                rows++;
            }
        }
        height += SUMMARY_TABLE_GAP + rows * SUMMARY_TABLE_LINE_HEIGHT;
    }
    return height;
}

/// Draws the end-of-drill summary: the drill name, score, pass mark, pass or
/// fail, then the option table (add_option_table). The summary menu
/// (tech_chase.c) sits to its left.
void training_summary_show(const Drill* drill)
{
    char line[TRAINING_TEXT_MAX_CHARS + 1];

    training_summary_hide();
    training_panel_show(&summary_panel, SUMMARY_X - SUMMARY_PANEL_PAD_X,
                        SUMMARY_Y - SUMMARY_PANEL_PAD_Y, SUMMARY_PANEL_W,
                        summary_height(drill) + 2.0F * SUMMARY_PANEL_PAD_Y +
                            SUMMARY_PANEL_BOTTOM_EXTRA);
    summary_text = training_text_create();
    summary_y = SUMMARY_Y;
    training_text_add(summary_text, SUMMARY_X, summary_y, SUMMARY_TITLE_SCALE,
                      SUMMARY_TITLE_SCALE, drill->name);
    summary_y += SUMMARY_TITLE_HEIGHT + SUMMARY_TITLE_GAP;
    format_fraction(line, "Score", successes, counted);
    add_summary_line(line);
    if (training_drill_has_pass_mark(drill)) {
        format_fraction(line, "Pass mark", drill->pass_mark,
                        drill->length_kind == DrillLength_Time
                            ? counted
                            : drill->length);
        add_summary_line(line);
        summary_y += SUMMARY_VERDICT_GAP;
        add_summary_line(training_score_passed(drill) ? "PASS" : "FAIL");
    }
    add_option_table();
}

/// Hides the summary, if shown.
void training_summary_hide(void)
{
    if (summary_text != NULL) {
        training_text_set_visible(summary_text, false);
        training_panel_set_visible(&summary_panel, false);
        summary_text = NULL;
    }
}

#endif
