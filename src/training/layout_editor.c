#ifdef TRAINING_BUILD

#include "layout_editor.h"

#include "drill.h"
#include "layout.h"
#include "menu.h"
#include "tech_chase.h"
#include "text.h"

#include <melee/gm/gm_1A36.h>
#include <melee/lb/lbaudio_ax.h>
#include <sysdolphin/baselib/controller.h>

#include <dolphin/pad.h>

#include <stdio.h>
#include <string.h>

/* Landing point and player start movement: game units per frame at full
 * stick tilt; tilts under the dead zone don't move it. Holding Z or a
 * trigger scales the sticks and the drop height adjustment by FINE_FACTOR.
 * Every move is whole tenths of a unit (at least one), so the edited values
 * stay in tenths (drill.h). */
#define MOVE_SPEED 1.0F
#define FINE_FACTOR 0.1F
#define STICK_DEAD_ZONE 0.2F
#define TRIGGER_HELD 0.3F
/* Stick up or down past this tilt steps the landing point between surfaces. */
#define STEP_TILT 0.6F

/* Drop height adjustment: game units per frame while the d-pad is held
 * (times FINE_FACTOR for fine), within [HEIGHT_MIN, HEIGHT_MAX] above the
 * landing surface. */
#define HEIGHT_SPEED 1.0F
#define HEIGHT_MIN 0.0F
#define HEIGHT_MAX 200.0F

/* The editor's text, in canvas units (as the pause menu's). */
#define TEXT_X 150.0F
#define TEXT_TITLE_Y 90.0F
#define TEXT_HELP_Y 125.0F
#define TEXT_TITLE_SCALE 0.9F
#define TEXT_HELP_SCALE 0.65F
#define TEXT_HEIGHTS_HELP_Y 147.0F
/* The current values, one line each below the help. */
#define TEXT_VALUES_Y 177.0F
#define TEXT_VALUES_STEP 22.0F

static HSD_Text* text;
/* The values lines: their text indices and what they show now (so a line is
 * only rewritten when it changes). */
enum { Value_Landing, Value_Heights, Value_Start, VALUE_LINES };
static int value_line[VALUE_LINES];
static char value_shown[VALUE_LINES][TRAINING_TEXT_MAX_CHARS + 1];
/* The drop height the opponent marker hangs at (an index into heights). */
static int selected_height;
/* The stick is tilted past STEP_TILT and has already stepped. */
static bool step_held;
/* The session drill as the editor opened: leaving restarts the drill only
 * if its layout differs from this. */
static Drill opened;

static void show_text(bool visible)
{
    int i;

    if (text == NULL) {
        if (!visible) {
            return;
        }
        text = training_text_create();
        training_text_add(text, TEXT_X, TEXT_TITLE_Y, TEXT_TITLE_SCALE,
                          TEXT_TITLE_SCALE, "Edit layout");
        training_text_add(text, TEXT_X, TEXT_HELP_Y, TEXT_HELP_SCALE,
                          TEXT_HELP_SCALE, "Stick:land C:start Z:fine B:done");
        training_text_add(text, TEXT_X, TEXT_HEIGHTS_HELP_Y, TEXT_HELP_SCALE,
                          TEXT_HELP_SCALE,
                          "D-pad:heights X:add Y:remove A:revert");
        for (i = 0; i < VALUE_LINES; i++) {
            value_shown[i][0] = 0;
            value_line[i] = training_text_add(
                text, TEXT_X, TEXT_VALUES_Y + i * TEXT_VALUES_STEP,
                TEXT_HELP_SCALE, TEXT_HELP_SCALE, " ");
        }
    }
    training_text_set_visible(text, visible);
}

static void set_value(int which, const char* ascii)
{
    if (strcmp(value_shown[which], ascii) != 0) {
        strncpy(value_shown[which], ascii, TRAINING_TEXT_MAX_CHARS);
        value_shown[which][TRAINING_TEXT_MAX_CHARS] = 0;
        training_text_set(text, value_line[which], ascii);
    }
}

/// Shows the session drill's landing x, drop heights (the selected one in
/// brackets) and player start offset (x and y).
static void show_values(void)
{
    const Drill* drill = training_drill_current();
    char line[TRAINING_TEXT_MAX_CHARS + 1];
    char* p;
    int i;

    p = line + sprintf(line, "Landing x ");
    training_drill_format_tenths(p, drill->landing_x);
    set_value(Value_Landing, line);

    p = line + sprintf(line, drill->height_kind == DrillHeight_Range
                                 ? "Height range"
                                 : "Heights");
    for (i = 0; i < drill->height_count; i++) {
        *p++ = ' ';
        if (i == selected_height) {
            *p++ = '[';
        }
        p = training_drill_format_tenths(p, drill->heights[i]);
        if (i == selected_height) {
            *p++ = ']';
        }
    }
    *p = 0;
    set_value(Value_Heights, line);

    p = line + sprintf(line, "Player start x ");
    p = training_drill_format_tenths(p, drill->start_dx);
    p += sprintf(p, " y ");
    training_drill_format_tenths(p, drill->start_dy);
    set_value(Value_Start, line);
}

void training_layout_editor_init(void)
{
    text = NULL;
    selected_height = 0;
}

void training_layout_editor_open(void)
{
    if (selected_height >= training_drill_current()->height_count) {
        selected_height = 0;
    }
    step_held = true;
    opened = *training_drill_current();
    training_tech_chase_edit_begin();
    training_tech_chase_edit_place(selected_height, NULL);
    show_text(true);
    show_values();
}

/// `value` moved by `delta` in whole tenths: `delta` rounded to tenths, but
/// at least one tenth in its direction.
static float step_tenths(float value, float delta)
{
    float step = training_drill_round_tenths(delta);

    if (step == 0.0F) {
        step = delta > 0.0F ? 0.1F : -0.1F;
    }
    return training_drill_round_tenths(value + step);
}

/// Moves one of the session drill's stick-driven values (the landing x or
/// the start offset's x or y) by `delta`, with the markers snapped onto the
/// surfaces below, and puts the resolved layout in `out` (if not NULL). A move the layout
/// cannot take (no floor below the landing point or the start) is undone, so
/// the value stays at its last valid position; false then.
static bool move_value(float* value, float delta, Layout* out)
{
    float old_value = *value;

    *value = step_tenths(*value, delta);
    if (training_tech_chase_edit_place(selected_height, out) != Layout_Ok) {
        *value = old_value;
        return false;
    }
    return true;
}

/// Moves the landing x by `dx` (see move_value).
static void move_landing(float dx)
{
    Drill* drill = training_drill_session();
    Layout layout;

    if (move_value(&drill->landing_x, dx, &layout)) {
        /* Keep the y hint on the surface found, so the next move snaps from
         * there (stepping off a platform lands on the floor below). */
        drill->landing_y = training_drill_round_tenths(layout.landing.y);
    }
}

/// Places the markers again, the opponent at the selected height.
static void place_markers(void)
{
    training_tech_chase_edit_place(selected_height, NULL);
}

/// D-pad left/right selects a height (a range's min and max are two
/// heights); up/down raises or lowers it. A range's min stays at or below
/// its max.
static void edit_heights(u64 pressed, u64 held, float speed)
{
    Drill* drill = training_drill_session();
    float lo = HEIGHT_MIN;
    float hi = HEIGHT_MAX;
    float* height;

    if ((pressed & PAD_BUTTON_LEFT) && selected_height > 0) {
        selected_height--;
        place_markers();
    }
    if ((pressed & PAD_BUTTON_RIGHT) &&
        selected_height < drill->height_count - 1)
    {
        selected_height++;
        place_markers();
    }
    if (!(held & (PAD_BUTTON_UP | PAD_BUTTON_DOWN))) {
        return;
    }
    height = &drill->heights[selected_height];
    *height = step_tenths(*height, (held & PAD_BUTTON_UP) ? speed : -speed);
    if (drill->height_kind == DrillHeight_Range) {
        if (selected_height == 0) {
            hi = drill->heights[1];
        } else {
            lo = drill->heights[0];
        }
    }
    if (*height < lo) {
        *height = lo;
    }
    if (*height > hi) {
        *height = hi;
    }
    place_markers();
}

/// X adds a copy of the selected height after it and selects the copy; Y
/// removes the selected height, never the last one. A range always has
/// exactly its min and max, so neither applies to it.
static void add_remove_height(u64 pressed)
{
    Drill* drill = training_drill_session();
    int i;

    if (drill->height_kind != DrillHeight_List) {
        return;
    }
    if ((pressed & PAD_BUTTON_X) && drill->height_count < DRILL_MAX_HEIGHTS) {
        for (i = drill->height_count; i > selected_height + 1; i--) {
            drill->heights[i] = drill->heights[i - 1];
        }
        drill->heights[selected_height + 1] = drill->heights[selected_height];
        drill->height_count++;
        selected_height++;
        place_markers();
    } else if ((pressed & PAD_BUTTON_Y) && drill->height_count > 1) {
        for (i = selected_height; i < drill->height_count - 1; i++) {
            drill->heights[i] = drill->heights[i + 1];
        }
        drill->height_count--;
        if (selected_height >= drill->height_count) {
            selected_height = drill->height_count - 1;
        }
        place_markers();
    }
}

/// Steps the landing point onto the next surface above (`up`) or below it,
/// at the same x. Nothing happens when there is none, or when the layout
/// cannot take it.
static void step_landing(bool up)
{
    Drill* drill = training_drill_session();
    float old_y = drill->landing_y;
    float new_y;
    Layout layout;

    if (training_tech_chase_edit_place(selected_height, &layout) != Layout_Ok ||
        !training_layout_step_surface(layout.landing.x, layout.landing.y, up,
                                      &new_y))
    {
        return;
    }
    drill->landing_y = new_y;
    if (training_tech_chase_edit_place(selected_height, &layout) != Layout_Ok)
    {
        drill->landing_y = old_y;
        training_tech_chase_edit_place(selected_height, NULL);
        return;
    }
    drill->landing_y = training_drill_round_tenths(layout.landing.y);
}

/// One step per stick flick: up or down past STEP_TILT steps once, and the
/// stick must come back inside it before the next step.
static void update_step(float stick_y)
{
    if (stick_y > STEP_TILT || stick_y < -STEP_TILT) {
        if (!step_held) {
            step_landing(stick_y > 0.0F);
        }
        step_held = true;
    } else {
        step_held = false;
    }
}

/// A puts the compiled drill's layout back, discarding the session's edits,
/// and places the markers there.
static void revert(void)
{
    training_drill_revert_layout();
    if (selected_height >= training_drill_current()->height_count) {
        selected_height = 0;
    }
    place_markers();
}

bool training_layout_editor_update(int port)
{
    u64 pressed = gm_GetButtonsTriggered((u8) port);
    HSD_PadStatus* pad = &HSD_PadCopyStatus[port];
    float stick = pad->nml_stickX;
    float cstick_x = pad->nml_subStickX;
    float cstick_y = pad->nml_subStickY;
    float fine = 1.0F;
    Drill* drill = training_drill_session();

    update_step(pad->nml_stickY);

    if (pressed & (PAD_BUTTON_B | PAD_BUTTON_START)) {
        lbAudioAx_80024030(TRAINING_MENU_SOUND_BACK);
        show_text(false);
        training_tech_chase_edit_end(
            !training_drill_same_layout(&opened, training_drill_current()));
        return false;
    }
    if ((pad->button & PAD_TRIGGER_Z) || pad->nml_analogL > TRIGGER_HELD ||
        pad->nml_analogR > TRIGGER_HELD)
    {
        fine = FINE_FACTOR;
    }
    if (stick > STICK_DEAD_ZONE || stick < -STICK_DEAD_ZONE) {
        move_landing(stick * MOVE_SPEED * fine);
    }
    if (cstick_x > STICK_DEAD_ZONE || cstick_x < -STICK_DEAD_ZONE) {
        move_value(&drill->start_dx, cstick_x * MOVE_SPEED * fine, NULL);
    }
    if (cstick_y > STICK_DEAD_ZONE || cstick_y < -STICK_DEAD_ZONE) {
        move_value(&drill->start_dy, cstick_y * MOVE_SPEED * fine, NULL);
    }
    edit_heights(pressed, pad->button, HEIGHT_SPEED * fine);
    add_remove_height(pressed);
    if (pressed & PAD_BUTTON_A) {
        revert();
    }
    show_values();
    return true;
}

#endif
