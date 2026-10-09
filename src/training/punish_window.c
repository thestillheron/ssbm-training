#ifdef TRAINING_BUILD

#include "punish_window.h"

#include "drill.h"
#include "slots.h"
#include "text.h"

#include <melee/ft/forward.h>
#include <melee/ft/inlines.h>
#include <melee/pl/player.h>

#include <stdio.h>

/* Lengths in frames; 0 is "not measured" (a window always has a frame). */
static short lengths[Ft_Kind_Max][RepTech_Count];

static bool in_range(int opponent_kind, RepTech tech)
{
    return opponent_kind >= 0 && opponent_kind < Ft_Kind_Max && tech >= 0 &&
           tech < RepTech_Count;
}

void training_punish_window_record(int opponent_kind, RepTech tech,
                                   int frames)
{
    if (!in_range(opponent_kind, tech) || frames <= 0) {
        return;
    }
    lengths[opponent_kind][tech] = (short) frames;
}

int training_punish_window(int opponent_kind, RepTech tech)
{
    if (!in_range(opponent_kind, tech) || lengths[opponent_kind][tech] == 0) {
        return NO_FRAME;
    }
    return lengths[opponent_kind][tech];
}

/* The panel, right of the menu (canvas units, tuned by eye). */
#define PANEL_X 420.0F
#define PANEL_Y 90.0F
#define PANEL_LINE_HEIGHT 24.0F
#define PANEL_SCALE 0.65F

/* The panel's text: a heading, then one line per tech in the drill's pool.
 * Built the first time it is shown in a match. */
static HSD_Text* panel_text;
static int panel_lines;
static RepTech panel_techs[TechOption_Count];

int training_punish_window_opponent_kind(void)
{
    HSD_GObj* opp = Player_GetEntity(OPPONENT_SLOT);

    return opp != NULL ? GET_FIGHTER(opp)->kind : -1;
}

static void format_window(char* dst, RepTech tech)
{
    int frames =
        training_punish_window(training_punish_window_opponent_kind(), tech);

    if (frames == NO_FRAME) {
        /* The SIS conversion has ASCII only: no em dash. */
        sprintf(dst, "%s: --", training_rep_tech_short_label(tech));
    } else {
        sprintf(dst, "%s: %df", training_rep_tech_short_label(tech), frames);
    }
}

void training_punish_window_panel_show(void)
{
    char line[TRAINING_TEXT_MAX_CHARS + 1];
    int i;

    if (panel_text == NULL) {
        int pool = training_drill_current()->option_pool;

        panel_text = training_text_create();
        training_text_add(panel_text, PANEL_X, PANEL_Y, PANEL_SCALE,
                          PANEL_SCALE, "Punish window");
        panel_lines = 0;
        for (i = 0; i < TechOption_Count; i++) {
            if (pool & TECH_OPTION_BIT(i)) {
                panel_techs[panel_lines] =
                    training_rep_tech_of_option((TechOption) i);
                panel_lines++;
                training_text_add(panel_text, PANEL_X,
                                  PANEL_Y + PANEL_LINE_HEIGHT * panel_lines,
                                  PANEL_SCALE, PANEL_SCALE, " ");
            }
        }
    }
    for (i = 0; i < panel_lines; i++) {
        format_window(line, panel_techs[i]);
        training_text_set(panel_text, 1 + i, line);
    }
    training_text_set_visible(panel_text, true);
}

void training_punish_window_panel_hide(void)
{
    if (panel_text != NULL) {
        training_text_set_visible(panel_text, false);
    }
}

void training_punish_window_panel_reset(void)
{
    panel_text = NULL;
    panel_lines = 0;
}

int training_punish_window_panel_lines(void)
{
    return panel_lines;
}

RepTech training_punish_window_panel_tech(int line)
{
    return panel_techs[line];
}

#endif
