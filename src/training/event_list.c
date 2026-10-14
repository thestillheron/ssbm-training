#ifdef TRAINING_BUILD

#include "event_list.h"

#include <melee/ft/forward.h>
#include <melee/lb/lbspdisplay.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>

#include <stdio.h>
#include <string.h>

#include "drill.h"
#include "main_menu.h"
#include "text.h"

/* The retail screen's layout (mnevent.c): 9 rows; a row's number and name
 * are offset from the row's position; the description box. */
#define LIST_ROWS 9
#define ROW_FONT_SIZE 0.035F
#define ROW_NUMBER_DX -3.8F
#define ROW_NAME_DX 1.0F
#define ROW_DY -0.6F
#define ROW_Z 17.0F
#define DESC_X -9.5F
#define DESC_Y 8.0F
#define DESC_FONT_SIZE 0.03F
/* Description lines: characters per line, the line step in the text's own
 * units, and how many lines the box holds, the pass mark included. A
 * description needing more than DESC_MAX_LINES - 1 lines is cut with "...",
 * so keep drill descriptions to two lines of DESC_WRAP characters. */
#define DESC_WRAP 32
#define DESC_LINE_STEP 30.0F
#define DESC_MAX_LINES 3
/* The screen's joints: the retail event's score kind label ([TIME] or
 * [KO]). */
#define JOINT_SCORE_KIND 9

/* The screen's top-level joints that make up the list: the scroll arrows
 * (1, with 2 and 3), the cell column (4) and its frame (5), the description
 * box (6, with 7 to 9), the cursor (10, with 11) and the row marker (12).
 * Hiding these leaves the menu background. */
static const int LIST_JOINTS[] = { 1, 4, 6, 10, 12 };

/* The retail text canvas the screen draws on. */
#define MENU_CANVAS 1

static GXColor row_number_colour = { 0xCA, 0xBC, 0x9F, 0xFF };

int training_event_list_last_top(void)
{
    int count = training_drill_event_count();

    return count > LIST_ROWS ? count - LIST_ROWS : 0;
}

void training_event_list_row(MnEventData* data, int line, int event,
                             const Vec3* pos)
{
    HSD_Text* text;
    char number[8];

    training_text_free(&data->texts[line]);
    training_text_free(&data->icons[line]);
    if (event < 0 || event >= training_drill_event_count()) {
        return;
    }

    text = training_text_create_on(MENU_CANVAS);
    data->texts[line] = text;
    training_text_place(text, pos->x + ROW_NUMBER_DX, pos->y + ROW_DY, ROW_Z,
                        ROW_FONT_SIZE);
    training_text_set_default_colour(text, row_number_colour);
    sprintf(number, "%d", event + 1);
    training_text_add_line(text, 0.0F, number);

    text = training_text_create_on(MENU_CANVAS);
    data->icons[line] = text;
    training_text_place(text, pos->x + ROW_NAME_DX, pos->y + ROW_DY, ROW_Z,
                        ROW_FONT_SIZE);
    training_text_add_line(text, 0.0F, training_drill_at(event)->name);
}

/// Copies the next line of `*rest` (at most DESC_WRAP characters, broken at
/// a space when it has to be) into `line` and moves `*rest` past it.
static void next_wrapped_line(char* line, const char** rest)
{
    const char* s = *rest;
    int len = 0;
    int cut = -1;
    int i;

    while (s[len] != 0 && len < DESC_WRAP) {
        len++;
    }
    if (s[len] != 0) {
        for (i = len; i > 0; i--) {
            if (s[i] == ' ') {
                cut = i;
                break;
            }
        }
        if (cut > 0) {
            len = cut;
        }
    }
    for (i = 0; i < len; i++) {
        line[i] = s[i];
    }
    line[len] = 0;
    s += len;
    while (*s == ' ') {
        s++;
    }
    *rest = s;
}

static void show_description(MnEventData* data, const Drill* drill)
{
    HSD_Text* text;
    char line[TRAINING_TEXT_MAX_CHARS + 1];
    const char* rest = drill->description;
    int n = 0;

    training_text_free(&data->desc_text);
    text = training_text_create_on(MENU_CANVAS);
    data->desc_text = text;
    training_text_place(text, DESC_X, DESC_Y, ROW_Z, DESC_FONT_SIZE);
    while (*rest != 0 && n < DESC_MAX_LINES - 1) {
        next_wrapped_line(line, &rest);
        if (*rest != 0 && n == DESC_MAX_LINES - 2) {
            /* No room for the rest: show that it was cut. */
            int len = (int) strlen(line);

            if (len > DESC_WRAP - 3) {
                len = DESC_WRAP - 3;
            }
            strcpy(line + len, "...");
        }
        training_text_add_line(text, n * DESC_LINE_STEP, line);
        n++;
    }
    if (!training_drill_has_pass_mark(drill)) {
        strcpy(line, "No pass mark");
    } else if (drill->length_kind == DrillLength_Time) {
        sprintf(line, "Pass mark: %d in %d s", drill->pass_mark,
                drill->length);
    } else {
        sprintf(line, "Pass mark: %d/%d", drill->pass_mark, drill->length);
    }
    if (drill->history) {
        /* The hint shares the pass mark's line: the box has no room for a
         * fourth. */
        strcpy(line + strlen(line), "  X: History");
    }
    training_text_add_line(text, n * DESC_LINE_STEP, line);
}

void training_event_list_show(HSD_GObj* gobj)
{
    MnEventData* data = gobj->user_data;
    int event = data->first_event + data->page;
    HSD_JObj* jobj;

    training_main_menu_heading(training_drill_is_custom(event));
    if (event >= training_drill_event_count()) {
        event = training_drill_event_count() - 1;
    }
    /* No retail event portrait, best score or score kind. */
    mnEvent_8024D0CC(gobj, ChKind_None);
    training_text_free(&data->name_text);
    data->name_text = training_text_create_on(MENU_CANVAS);
    lb_80011E24(gobj->hsd_obj, &jobj, JOINT_SCORE_KIND, -1);
    HSD_JObjSetFlagsAll(jobj, JOBJ_HIDDEN);

    show_description(data, training_drill_at(event));
    mnEvent_8024D014(gobj);
}

bool training_event_list_at_end(const MnEventData* data)
{
    return data->first_event + data->page >= training_drill_event_count() - 1;
}

void training_event_list_set_visible(MnEventData* data, bool visible)
{
    int i;

    for (i = 0; i < LIST_ROWS; i++) {
        if (data->texts[i] != NULL) {
            training_text_set_visible(data->texts[i], visible);
        }
        if (data->icons[i] != NULL) {
            training_text_set_visible(data->icons[i], visible);
        }
    }
    if (data->desc_text != NULL) {
        training_text_set_visible(data->desc_text, visible);
    }
}

void training_event_list_hide(HSD_GObj* gobj)
{
    MnEventData* data = gobj->user_data;
    HSD_JObj* jobj;
    int i;

    training_event_list_set_visible(data, false);
    for (i = 0; i < (int) (sizeof(LIST_JOINTS) / sizeof(LIST_JOINTS[0])); i++) {
        lb_80011E24(gobj->hsd_obj, &jobj, LIST_JOINTS[i], -1);
        HSD_JObjSetFlagsAll(jobj, JOBJ_HIDDEN);
    }
}

#endif
