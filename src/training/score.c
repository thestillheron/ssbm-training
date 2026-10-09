#ifdef TRAINING_BUILD

#include "score.h"

#include "drill.h"
#include "text.h"

#include <sysdolphin/baselib/sislib.h>

#include <stdio.h>

/* Position (canvas units) and size of the score text: top left, clear
 * of the rep result above the damage HUD. Tune by
 * eye with `run`.
 * Line one is successes out of counted reps, line two the success rate. */
#define SCORE_X 30.0F
#define SCORE_Y 75.0F
#define SCORE_LINE_HEIGHT 28.0F
#define SCORE_SCALE_X 1.0F
#define SCORE_SCALE_Y 0.8F

static int successes;
static int counted;
/* The same counts per tech the opponent did, with early and late failures,
 * for the end-of-drill summary. */
static TechScore techs[RepTech_Count];
static HSD_Text* score_text;
static int line_count_idx;
static int line_rate_idx;

/* The drill summary: lines of text near the screen centre, then a table of
 * the options that came up (one row each, under a header), smaller, so its
 * four columns fit across. Column x positions fit the widest cells: "In
 * place", "10/10 100%", "Early", "Late" (about 15 canvas units a character
 * at the table's scale). The gap after the table keeps the larger last line
 * clear of it. Tune by eye with `run`. */
#define SUMMARY_X 265.0F
#define SUMMARY_Y 90.0F
#define SUMMARY_LINE_HEIGHT 30.0F
#define SUMMARY_SCALE_X 1.0F
#define SUMMARY_SCALE_Y 0.9F
#define SUMMARY_TABLE_LINE_HEIGHT 24.0F
#define SUMMARY_TABLE_GAP 12.0F
#define SUMMARY_TABLE_SCALE_X 0.75F
#define SUMMARY_TABLE_SCALE_Y 0.75F
#define SUMMARY_COLUMNS 4

static const float summary_column_x[SUMMARY_COLUMNS] = { SUMMARY_X, 372.0F,
                                                          497.0F, 570.0F };

static HSD_Text* summary_text;
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

    if (score_text == NULL) {
        return;
    }
    sprintf(line, "%d/%d", successes, counted);
    training_text_set(score_text, line_count_idx, line);
    sprintf(line, "%d%%", success_percent());
    training_text_set(score_text, line_rate_idx, line);
}

/// Zeroes the score. Called when a drill event starts.
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

/// True when the successes so far reach the drill's pass mark.
bool training_score_passed(const Drill* drill)
{
    return successes >= drill->pass_mark;
}

/// Creates the score text, once the scene has settled (text made during the
/// scene's own setup is not drawn). Each line starts short and grows as the
/// counts do.
void training_score_show(void)
{
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
    training_text_add(summary_text, SUMMARY_X, summary_y, SUMMARY_SCALE_X,
                      SUMMARY_SCALE_Y, ascii);
    summary_y += SUMMARY_LINE_HEIGHT;
}

/// Adds one row of the summary's option table, a cell per column.
static void add_summary_row(const char* const cells[SUMMARY_COLUMNS])
{
    int i;

    for (i = 0; i < SUMMARY_COLUMNS; i++) {
        training_text_add(summary_text, summary_column_x[i], summary_y,
                          SUMMARY_TABLE_SCALE_X, SUMMARY_TABLE_SCALE_Y,
                          cells[i]);
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
    summary_y += SUMMARY_TABLE_GAP;
}

/// Draws the end-of-drill summary: the drill name, score, pass mark, pass or
/// fail, then the option table (add_option_table). The summary menu
/// (tech_chase.c) sits to its left.
void training_summary_show(const Drill* drill)
{
    char line[TRAINING_TEXT_MAX_CHARS + 1];

    training_summary_hide();
    summary_text = training_text_create();
    summary_y = SUMMARY_Y;
    add_summary_line(drill->name);
    format_fraction(line, "Score", successes, counted);
    add_summary_line(line);
    format_fraction(line, "Pass mark", drill->pass_mark,
                    drill->length_kind == DrillLength_Time ? counted
                                                           : drill->length);
    add_summary_line(line);
    add_summary_line(training_score_passed(drill) ? "PASS" : "FAIL");
    add_option_table();
}

/// Hides the summary, if shown.
void training_summary_hide(void)
{
    if (summary_text != NULL) {
        training_text_set_visible(summary_text, false);
        summary_text = NULL;
    }
}

#endif
