#ifdef TRAINING_BUILD

#include "custom_settings.h"

#include <melee/mn/inlines.h>
#include <melee/mn/mnmain.h>
#include <sysdolphin/baselib/sislib.h>

#include <stdio.h>
#include <string.h>

#include "drill.h"
#include "event_list.h"
#include "menu.h"
#include "tech_options.h"
#include "text.h"

#ifdef AUTHORING_BUILD
#include <dolphin/os.h>
#endif

/* The on-screen message for a refused change: below the menu, shown for this
 * many frames. */
#define MESSAGE_X 150.0F
#define MESSAGE_Y 285.0F
#define MESSAGE_SCALE 0.75F
#define MESSAGE_FRAMES 150
/* The backdrop behind it: padding round the text, and its height. */
#define MESSAGE_PAD_X 14.0F
#define MESSAGE_PAD_Y 8.0F
#define MESSAGE_HEIGHT 40.0F
#define MESSAGE_CHAR_WIDTH 20.0F
static const GXColor message_colour = { 255, 90, 90, 255 };

/* Row text: "name: value". */
#define LABEL_CHARS 40
#define VALUE_CHARS 16

/* One settings row. `value` writes the current value ("On") for the label;
 * NULL for a row with none (Start). `activate` runs when A is pressed on the
 * row; it edits the custom drill, and may call refuse(). `enabled` says
 * whether the row can be chosen now (greyed and skipped when not); NULL for
 * a row that always can. */
typedef struct SettingsRow {
    const char* name;
    const char* description;
    void (*value)(char* out);
    void (*activate)(void);
    bool (*enabled)(void);
} SettingsRow;

static bool start_chosen;
static const char* pending_message;

/// Shows `message` on screen for a moment: the change was refused.
static void refuse(const char* message)
{
    pending_message = message;
}

static void activate_start(void)
{
    start_chosen = true;
}

static void on_off(char* out, bool on)
{
    strcpy(out, on ? "On" : "Off");
}

/* Tech options: one row each, toggling the option in the pool. The pool
 * can't be emptied. */
static bool option_on(TechOption option)
{
    return (training_drill_custom()->option_pool & TECH_OPTION_BIT(option)) !=
           0;
}

static void toggle_option(TechOption option)
{
    Drill* drill = training_drill_custom();
    int pool = drill->option_pool ^ TECH_OPTION_BIT(option);

    if (pool == 0) {
#ifdef AUTHORING_BUILD
        OSReport("[rep] event=custom_refused reason=empty_pool\n");
#endif
        refuse("Keep at least one option on");
        return;
    }
    drill->option_pool = pool;
}

static void value_in_place(char* out)
{
    on_off(out, option_on(TechOption_InPlace));
}

static void value_in(char* out)
{
    on_off(out, option_on(TechOption_In));
}

static void value_away(char* out)
{
    on_off(out, option_on(TechOption_Away));
}

static void value_miss(char* out)
{
    on_off(out, option_on(TechOption_Miss));
}

static void toggle_in_place(void)
{
    toggle_option(TechOption_InPlace);
}

static void toggle_in(void)
{
    toggle_option(TechOption_In);
}

static void toggle_away(void)
{
    toggle_option(TechOption_Away);
}

static void toggle_miss(void)
{
    toggle_option(TechOption_Miss);
}

/* Drop heights. Fixed is a list of one. A list holds up to DRILL_MAX_HEIGHTS
 * heights, a range its two ends. Heights go in HEIGHT_STEP steps between
 * HEIGHT_MIN and HEIGHT_MAX, so every set the menu makes is usable; A on a
 * height row steps it up. */
#define HEIGHT_STEP 10
#define HEIGHT_MIN 10
#define HEIGHT_MAX 120
#define HEIGHT_NEW 40
#define HEIGHT_RANGE_GAP 30

typedef enum HeightMode {
    HeightMode_Fixed,
    HeightMode_List,
    HeightMode_Range,
    HeightMode_Count,
} HeightMode;

static HeightMode height_mode;

/// The height mode the custom drill's heights are in.
static HeightMode mode_of(const Drill* drill)
{
    if (drill->height_kind == DrillHeight_Range) {
        return HeightMode_Range;
    }
    return drill->height_count == 1 ? HeightMode_Fixed : HeightMode_List;
}

static void value_height_mode(char* out)
{
    static const char* const names[HeightMode_Count] = { "Fixed", "List",
                                                         "Range" };

    strcpy(out, names[height_mode]);
}

/// Next mode in the cycle, converting the custom drill's heights to it.
static void cycle_height_mode(void)
{
    Drill* drill = training_drill_custom();
    int i;

    height_mode = (HeightMode) ((height_mode + 1) % HeightMode_Count);
    switch (height_mode) {
    case HeightMode_Fixed:
        /* From a range: its low end. */
        drill->height_kind = DrillHeight_List;
        drill->height_count = 1;
        break;
    case HeightMode_List:
        /* The fixed height, as a list of one. */
        break;
    case HeightMode_Range:
        drill->height_kind = DrillHeight_Range;
        for (i = 1; i < drill->height_count; i++) {
            if (drill->heights[i] < drill->heights[0]) {
                drill->heights[0] = drill->heights[i];
            }
        }
        drill->heights[1] = drill->heights[0] + HEIGHT_RANGE_GAP;
        if (drill->heights[1] > HEIGHT_MAX) {
            drill->heights[1] = HEIGHT_MAX;
        }
        drill->height_count = 2;
        break;
    default:
        break;
    }
}

/// Writes the value of height row `n` (0 to DRILL_MAX_HEIGHTS - 1).
static void height_value(char* out, int n)
{
    const Drill* drill = training_drill_custom();

    if (n >= drill->height_count) {
        strcpy(out, height_mode == HeightMode_List ? "Off" : "-");
    } else if (height_mode == HeightMode_Range) {
        sprintf(out, "%d (%s)", (int) drill->heights[n],
                n == 0 ? "min" : "max");
    } else {
        sprintf(out, "%d", (int) drill->heights[n]);
    }
}

/// A on height row `n`: steps the height up. In a list, stepping past the
/// maximum removes it (the last one wraps instead, so the list never
/// empties) and A on an Off row adds a height. A range end wraps within its
/// limits, so min never passes max.
static void step_height(int n)
{
    Drill* drill = training_drill_custom();
    float lo = HEIGHT_MIN;
    float hi = HEIGHT_MAX;
    float next;
    int i;

    if (n >= drill->height_count) {
        if (height_mode == HeightMode_List) {
            drill->heights[drill->height_count++] = HEIGHT_NEW;
        }
        return;
    }
    if (height_mode == HeightMode_Range) {
        if (n == 0) {
            hi = drill->heights[1];
        } else {
            lo = drill->heights[0];
        }
    }
    next = drill->heights[n] + HEIGHT_STEP;
    if (next > hi) {
        if (height_mode == HeightMode_List && drill->height_count > 1) {
            for (i = n; i < drill->height_count - 1; i++) {
                drill->heights[i] = drill->heights[i + 1];
            }
            drill->height_count--;
            return;
        }
        next = lo;
    }
    drill->heights[n] = next;
}

/// Whether height row `n` is in use in the current mode: Fixed uses the
/// first, Range the first two (min and max), List all of them.
static bool height_row_enabled(int n)
{
    switch (height_mode) {
    case HeightMode_Fixed:
        return n < 1;
    case HeightMode_Range:
        return n < 2;
    default:
        return true;
    }
}

#define HEIGHT_ROW(n)                                                         \
    static void value_height_##n(char* out)                                   \
    {                                                                         \
        height_value(out, n);                                                 \
    }                                                                         \
    static bool enabled_height_##n(void)                                      \
    {                                                                         \
        return height_row_enabled(n);                                         \
    }                                                                         \
    static void step_height_##n(void)                                         \
    {                                                                         \
        step_height(n);                                                       \
    }
HEIGHT_ROW(0)
HEIGHT_ROW(1)
HEIGHT_ROW(2)
HEIGHT_ROW(3)
/* Length and pass mark. A cycles through a short list of values, wrapping to
 * the first; the length list depends on the kind (reps or seconds). Changing
 * the kind puts the default length of the new kind. A pass mark of 0 is none;
 * in a rep-count drill it can't exceed the length. */
static const int rep_lengths[] = { 5, 10, 15, 20, 30, 50 };
static const int time_lengths[] = { 30, 60, 90, 120, 180, 300 };
static const int pass_marks[] = { 0,  1,  2,  3,  4,  5,  7, 10,
                                  12, 15, 20, 25, 30, 40, 50 };
#define DEFAULT_REPS 10
#define DEFAULT_SECONDS 60
#define LIST_COUNT(list) ((int) (sizeof(list) / sizeof((list)[0])))

/// The entry after `current` in `list` (the first, if `current` is last or
/// not in the list).
static int next_in_list(const int* list, int count, int current)
{
    int i;

    for (i = 0; i < count - 1; i++) {
        if (list[i] == current) {
            return list[i + 1];
        }
    }
    return list[0];
}

static void limit_pass_mark(Drill* drill)
{
    if (drill->length_kind == DrillLength_Reps &&
        drill->pass_mark > drill->length)
    {
        drill->pass_mark = drill->length;
    }
}

static void value_length_kind(char* out)
{
    strcpy(out, training_drill_custom()->length_kind == DrillLength_Time
                    ? "Time"
                    : "Reps");
}

static void cycle_length_kind(void)
{
    Drill* drill = training_drill_custom();

    if (drill->length_kind == DrillLength_Time) {
        drill->length_kind = DrillLength_Reps;
        drill->length = DEFAULT_REPS;
    } else {
        drill->length_kind = DrillLength_Time;
        drill->length = DEFAULT_SECONDS;
    }
    limit_pass_mark(drill);
}

static void value_length(char* out)
{
    const Drill* drill = training_drill_custom();

    if (drill->length_kind == DrillLength_Time) {
        sprintf(out, "%d s", drill->length);
    } else {
        sprintf(out, "%d", drill->length);
    }
}

static void cycle_length(void)
{
    Drill* drill = training_drill_custom();

    if (drill->length_kind == DrillLength_Time) {
        drill->length = next_in_list(time_lengths, LIST_COUNT(time_lengths),
                                     drill->length);
    } else {
        drill->length = next_in_list(rep_lengths, LIST_COUNT(rep_lengths),
                                     drill->length);
    }
    limit_pass_mark(drill);
}

static void value_pass_mark(char* out)
{
    int pass_mark = training_drill_custom()->pass_mark;

    if (!training_drill_has_pass_mark(training_drill_custom())) {
        strcpy(out, "None");
    } else {
        sprintf(out, "%d", pass_mark);
    }
}

static void cycle_pass_mark(void)
{
    Drill* drill = training_drill_custom();
    int next = next_in_list(pass_marks, LIST_COUNT(pass_marks),
                            drill->pass_mark);

    if (drill->length_kind == DrillLength_Reps && next > drill->length) {
        next = 0;
    }
    drill->pass_mark = next;
}

/* Vanish, player reset and opponent facing: A cycles the value. */
static void value_vanish(char* out)
{
    on_off(out, training_drill_custom()->vanish);
}

static void toggle_vanish(void)
{
    Drill* drill = training_drill_custom();

    drill->vanish = !drill->vanish;
}

static void value_reset(char* out)
{
    on_off(out, training_drill_custom()->reset_player);
}

static void toggle_reset(void)
{
    Drill* drill = training_drill_custom();

    drill->reset_player = !drill->reset_player;
}

static void value_facing(char* out)
{
    switch (training_drill_custom()->opponent_facing) {
    case DrillFacing_Away:
        strcpy(out, "Away");
        break;
    case DrillFacing_Left:
        strcpy(out, "Left");
        break;
    case DrillFacing_Right:
        strcpy(out, "Right");
        break;
    default:
        strcpy(out, "Toward you");
        break;
    }
}

static void cycle_facing(void)
{
    Drill* drill = training_drill_custom();

    drill->opponent_facing =
        (DrillFacing) ((drill->opponent_facing + 1) % (DrillFacing_Right + 1));
}

/* The rows, in menu order. Later tickets add theirs here, one line each. */
static const SettingsRow rows[] = {
    { "Start", "Go on to the character select", NULL, activate_start },
    { "Tech in place", "Opponent may tech in place", value_in_place,
      toggle_in_place },
    { "Tech in", "Opponent may tech toward you", value_in, toggle_in },
    { "Tech away", "Opponent may tech away from you", value_away,
      toggle_away },
    { "Miss tech", "Opponent may miss the tech", value_miss, toggle_miss },
    { "Drop heights", "Fixed height, a list, or a range",
      value_height_mode, cycle_height_mode },
    { "Height 1", "A raises it; a list drops past top",
      value_height_0, step_height_0, enabled_height_0 },
    { "Height 2", "A raises it; a list drops past top",
      value_height_1, step_height_1, enabled_height_1 },
    { "Height 3", "A raises it; a list drops past top",
      value_height_2, step_height_2, enabled_height_2 },
    { "Height 4", "A raises it; a list drops past top",
      value_height_3, step_height_3, enabled_height_3 },
    { "Length type", "Count reps, or run for a time", value_length_kind,
      cycle_length_kind },
    { "Length", "How many reps, or how many seconds", value_length,
      cycle_length },
    { "Pass mark", "Successes to pass, or None",
      value_pass_mark, cycle_pass_mark },
    { "Vanish", "Opponent vanishes during the tech", value_vanish,
      toggle_vanish },
    { "Player reset", "Put you back at the start each rep", value_reset,
      toggle_reset },
    { "Opponent facing", "Which way the opponent faces",
      value_facing, cycle_facing },
};
#define ROW_COUNT ((int) (sizeof(rows) / sizeof(rows[0])))

static char labels[ROW_COUNT][LABEL_CHARS];
static TrainingMenuItem items[ROW_COUNT];
static TrainingMenu menu = { "Custom drill", items, ROW_COUNT };

static bool menu_open;
static int port;
static HSD_Text* message_text;
static TrainingPanel message_panel;
static int message_frames;

/// Writes each row's label from its name and current value, and its enabled
/// state, and returns true if any of them changed.
static bool refresh_labels(void)
{
    bool changed = false;
    int i;

    for (i = 0; i < ROW_COUNT; i++) {
        char label[LABEL_CHARS];
        bool enabled;

        if (rows[i].value != NULL) {
            char value[VALUE_CHARS];

            rows[i].value(value);
            sprintf(label, "%s: %s", rows[i].name, value);
        } else {
            strcpy(label, rows[i].name);
        }
        if (strcmp(label, labels[i]) != 0) {
            strcpy(labels[i], label);
            changed = true;
        }
        items[i].label = labels[i];
        items[i].description = rows[i].description;
        enabled = rows[i].enabled == NULL || rows[i].enabled();
        if (items[i].enabled != enabled) {
            items[i].enabled = enabled;
            changed = true;
        }
        items[i].confirm = rows[i].activate;
    }
    return changed;
}

static void show_message(const char* message)
{
    training_text_free(&message_text);
    training_panel_free(&message_panel);
    training_panel_show(&message_panel, MESSAGE_X - MESSAGE_PAD_X,
                        MESSAGE_Y - MESSAGE_PAD_Y,
                        strlen(message) * MESSAGE_SCALE * MESSAGE_CHAR_WIDTH +
                            2.0F * MESSAGE_PAD_X,
                        MESSAGE_HEIGHT);
    message_text = training_text_create();
    training_text_add(message_text, MESSAGE_X, MESSAGE_Y, MESSAGE_SCALE,
                      MESSAGE_SCALE, message);
    training_text_set_colour(message_text, 0, message_colour);
    message_frames = MESSAGE_FRAMES;
}

static void open_menu(MnEventData* data)
{
    menu_open = true;
    port = mn_802295AC();
    start_chosen = false;
    pending_message = NULL;
    height_mode = mode_of(training_drill_custom());
    training_menu_reset(&menu);
    refresh_labels();
    training_event_list_set_visible(data, false);
    training_menu_show(&menu);
}

static void close_menu(MnEventData* data)
{
    menu_open = false;
    training_menu_free(&menu);
    training_text_free(&message_text);
    training_panel_free(&message_panel);
    training_event_list_set_visible(data, true);
}

bool training_custom_settings_input(HSD_GObj* gobj, u64* inputs)
{
    MnEventData* data;
    TrainingMenuAction action;

    if (gobj == NULL) {
        return false;
    }
    data = gobj->user_data;

    if (!menu_open) {
        if ((*inputs & MenuInput_Confirm) &&
            training_drill_is_custom(data->first_event + data->page))
        {
            open_menu(data);
            return true;
        }
        return false;
    }

    action = training_menu_update(&menu, port);
    if (start_chosen) {
        close_menu(data);
        *inputs = MenuInput_Confirm;
        return false;
    }
    if (action == TrainingMenuAction_Back) {
        close_menu(data);
        return true;
    }
    if (pending_message != NULL) {
        show_message(pending_message);
        pending_message = NULL;
    }
    if (message_text != NULL && --message_frames <= 0) {
        training_text_free(&message_text);
        training_panel_free(&message_panel);
    }
    if (refresh_labels()) {
        training_menu_show(&menu);
    }
    return true;
}

#endif
