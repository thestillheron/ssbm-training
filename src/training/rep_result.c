#ifdef TRAINING_BUILD

#include "rep_result.h"

#include "text.h"

#include <melee/if/ifall.h>
#include <sysdolphin/baselib/sislib.h>

#include <stdio.h>

/* Slot of the player whose damage HUD the result sits above. */
#define RESULT_PLAYER_SLOT 0

/* The damage HUD position (ifAll_GetPlayerHUDPosition) is in the HUD's own
 * units, with y up; the text canvas has y down. canvas = origin + hud * scale,
 * fitted from shots of the two Training HUDs (P1 at (-21, -21), P2 at
 * (-7, -21)). */
#define HUD_CANVAS_ORIGIN_X 320.0F
#define HUD_CANVAS_ORIGIN_Y 229.0F
#define HUD_CANVAS_SCALE_X 10.7F
#define HUD_CANVAS_SCALE_Y (-9.1F)

/* Line 2 sits this many HUD units above the damage HUD (above its stock
 * icon), line 1 over it, both centred. Tune by eye with `run`. */
#define RESULT_HUD_Y_OFFSET 9.5F
#define RESULT_LINE_HEIGHT 22.0F
#define RESULT_SCALE_X 0.55F
#define RESULT_SCALE_Y 0.65F

/* The P1 HUD is near the left edge, so the centre is moved right just enough
 * for the longest line the result can show (longest_line) to stay on screen.
 * A character is about 20 canvas units wide at scale 1. */
#define RESULT_CHAR_WIDTH (20.0F * RESULT_SCALE_X)
#define RESULT_EDGE_MARGIN 12.0F

/* The largest number a timing line is sized for (frames, k or n). */
#define RESULT_SAMPLE_NUMBER 999

/* Each tech's names, in RepTech's order: the result's label, the summary's
 * short label and the rep log's name. */
static const struct {
    const char* label;
    const char* short_label;
    REP_LOG_ONLY(const char* log_name;)
} techs[RepTech_Count] = {
    { "Tech in place", "In place" REP_LOG_ONLY(, "in-place") },
    { "Tech in", "In" REP_LOG_ONLY(, "in") },
    { "Tech away", "Away" REP_LOG_ONLY(, "away") },
    { "Missed tech", "Miss" REP_LOG_ONLY(, "miss") },
    { "Wall tech", "Wall" REP_LOG_ONLY(, "wall") },
    { "Ceiling tech", "Ceiling" REP_LOG_ONLY(, "ceil") },
};

/* Each outcome's label and line 1 colour (UnclePunch's message colours: soft
 * green, red and yellow), and its rep log name, in RepOutcome's order. */
static const struct {
    const char* label;
    GXColor colour;
    REP_LOG_ONLY(const char* log_name;)
} outcomes[RepOutcome_Count] = {
    { "Success", { 141, 255, 110, 255 } REP_LOG_ONLY(, "success") },
    { "Failure", { 255, 162, 186, 255 } REP_LOG_ONLY(, "failure") },
    { "Void", { 255, 240, 0, 255 } REP_LOG_ONLY(, "void") },
};

static GXColor const colour_white = { 255, 255, 255, 255 };

/* Each timing class's line 2 and rep log value, in RepTimingKind's order, as
 * printf formats of two numbers: k and n for a success, else the frames early
 * or late (and 0). A success whose n is unknown shows `unknown_n_text`. */
static const struct {
    const char* text;
    REP_LOG_ONLY(const char* log_value;)
} timing_texts[RepTiming_Count] = {
    { "No attack" REP_LOG_ONLY(, "none") },
    { "Frame %d/%d" REP_LOG_ONLY(, "success_%d/%d") },
    { "Early: hit before landing" REP_LOG_ONLY(, "void") },
    { "%df early" REP_LOG_ONLY(, "early_%df") },
    { "%df late" REP_LOG_ONLY(, "late_%df") },
};
static const char unknown_n_text[] = "Frame %d";

static HSD_Text* result_text;
static int line_outcome;
static int line_timing;

/// The two numbers a timing class's formats print.
static void timing_numbers(const RepTiming* timing, int* a, int* b)
{
    if (timing->kind == RepTiming_Success) {
        *a = timing->k;
        *b = timing->n;
    } else {
        *a = timing->frames;
        *b = 0;
    }
}

RepTiming training_rep_classify(const RepFrames* f)
{
    RepTiming timing;

    timing.kind = RepTiming_None;
    timing.k = 0;
    timing.n = 0;
    timing.frames = 0;
    if (f->voided) {
        timing.kind = RepTiming_Void;
    } else if (f->hit.frame != NO_FRAME) {
        /* Vulnerable frames counted from 1, up to the predicted last one. */
        int first = f->vuln_from != NO_FRAME ? f->vuln_from : f->hit.frame;

        timing.kind = RepTiming_Success;
        timing.k = f->hit.frame - first + 1;
        if (f->predicted_vuln_to != NO_FRAME) {
            timing.n = f->predicted_vuln_to - first + 1;
        }
        if (timing.n < timing.k) {
            /* A prediction that falls short is wrong: n is unknown. */
            timing.n = 0;
        }
    } else if (f->late_hit.frame != NO_FRAME) {
        /* A late hit wins over an early whiff: the hit connected. */
        timing.kind = RepTiming_Late;
        timing.frames = f->late_hit.frame - f->actionable;
    } else if (f->early_frames != NO_FRAME) {
        timing.kind = RepTiming_Early;
        timing.frames = f->early_frames;
    }
    return timing;
}

RepTech training_rep_tech_of_option(TechOption option)
{
    /* RepTech starts with the tech options, in the same order. */
    return (RepTech) option;
}

const char* training_rep_tech_label(RepTech tech)
{
    return techs[tech].label;
}

const char* training_rep_tech_short_label(RepTech tech)
{
    return techs[tech].short_label;
}

#ifdef AUTHORING_BUILD
void training_rep_timing_log_value(char* dst, const RepTiming* timing)
{
    int a;
    int b;

    timing_numbers(timing, &a, &b);
    sprintf(dst, timing_texts[timing->kind].log_value, a, b);
}

const char* training_rep_tech_log_name(RepTech tech)
{
    return techs[tech].log_name;
}

const char* training_rep_outcome_log_name(RepOutcome outcome)
{
    return outcomes[outcome].log_name;
}
#endif

/// Writes line 2 of the result for `timing` into `dst`; returns its length.
static int timing_text(char* dst, const RepTiming* timing)
{
    const char* format = timing_texts[timing->kind].text;
    int a;
    int b;

    timing_numbers(timing, &a, &b);
    if (timing->kind == RepTiming_Success && timing->n <= 0) {
        format = unknown_n_text;
    }
    return sprintf(dst, format, a, b);
}

/// Writes line 1 of the result into `dst`; returns its length.
static int outcome_text(char* dst, RepTech tech, RepOutcome outcome)
{
    return sprintf(dst, "%s: %s", techs[tech].label, outcomes[outcome].label);
}

/// The longest line the result can show, in characters: every tech with
/// every outcome, and every timing class with numbers of up to
/// RESULT_SAMPLE_NUMBER.
static int longest_line(void)
{
    char line[TRAINING_TEXT_MAX_CHARS * 2];
    RepTiming timing;
    int longest = 0;
    int i;
    int j;
    int len;

    for (i = 0; i < RepTech_Count; i++) {
        for (j = 0; j < RepOutcome_Count; j++) {
            len = outcome_text(line, (RepTech) i, (RepOutcome) j);
            if (len > longest) {
                longest = len;
            }
        }
    }
    timing.k = timing.n = timing.frames = RESULT_SAMPLE_NUMBER;
    for (i = 0; i < RepTiming_Count; i++) {
        timing.kind = (RepTimingKind) i;
        len = timing_text(line, &timing);
        if (len > longest) {
            longest = len;
        }
    }
    return longest;
}

/// Creates the result text, hidden, above the player's damage HUD. The HUD
/// has its place once the scene has settled, which is also when text can be
/// made.
void training_rep_result_create(void)
{
    Vec3* hud = ifAll_GetPlayerHUDPosition(RESULT_PLAYER_SLOT);
    float x = HUD_CANVAS_ORIGIN_X + hud->x * HUD_CANVAS_SCALE_X;
    float y = HUD_CANVAS_ORIGIN_Y +
              (hud->y + RESULT_HUD_Y_OFFSET) * HUD_CANVAS_SCALE_Y;
    float min_x =
        RESULT_EDGE_MARGIN + longest_line() * RESULT_CHAR_WIDTH / 2;

    if (x < min_x) {
        x = min_x;
    }
    result_text = training_text_create();
    training_text_centre(result_text);
    line_outcome =
        training_text_add(result_text, x, y - RESULT_LINE_HEIGHT,
                          RESULT_SCALE_X, RESULT_SCALE_Y, " ");
    line_timing = training_text_add(result_text, x, y, RESULT_SCALE_X,
                                    RESULT_SCALE_Y, " ");
    training_text_set_colour(result_text, line_timing, colour_white);
    training_text_set_visible(result_text, false);
}

/// Shows the rep's result: line 1 the tech and the outcome, coloured by the
/// outcome; line 2 the timing in white.
void training_rep_result_show(RepTech tech, RepOutcome outcome,
                              const RepTiming* timing)
{
    char line[TRAINING_TEXT_MAX_CHARS * 2];

    if (result_text == NULL) {
        return;
    }
    outcome_text(line, tech, outcome);
    training_text_set(result_text, line_outcome, line);
    training_text_set_colour(result_text, line_outcome,
                             outcomes[outcome].colour);
    timing_text(line, timing);
    training_text_set(result_text, line_timing, line);
    training_text_set_visible(result_text, true);
}

void training_rep_result_hide(void)
{
    if (result_text != NULL) {
        training_text_set_visible(result_text, false);
    }
}

/// Forgets the text, which the scene frees on exit. Called when Training is
/// entered.
void training_rep_result_init(void)
{
    result_text = NULL;
}

#endif
