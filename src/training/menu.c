#ifdef TRAINING_BUILD

#include "menu.h"

#include "text.h"

#include <melee/gm/gm_1A36.h>
#include <melee/lb/lbaudio_ax.h>

#include <dolphin/pad.h>

#ifdef AUTHORING_BUILD
#include <dolphin/os.h>
#endif

/* Rows shown at once; the list scrolls past that. */
#define MENU_ROWS 9

/* Layout in canvas units, tuned by eye with `run`: the title, then the rows,
 * then the highlighted item's description under the last row shown. */
#define MENU_X 150.0F
#define MENU_TITLE_Y 90.0F
#define MENU_ROW_Y 125.0F
#define MENU_ROW_HEIGHT 24.0F
#define MENU_DESCRIPTION_GAP 10.0F
#define MENU_TITLE_SCALE 0.9F
#define MENU_ROW_SCALE 0.75F
#define MENU_DESCRIPTION_SCALE 0.65F

/* Sounds of lbAudioAx_80024030, the game's menu sounds. */
#define SOUND_BACK 0
#define SOUND_CONFIRM 1
#define SOUND_MOVE 2

static const GXColor colour_normal = { 255, 255, 255, 255 };
static const GXColor colour_highlight = { 255, 240, 0, 255 };
static const GXColor colour_disabled = { 120, 120, 120, 255 };

/* Line indices in the menu's text: the title, the rows, the description. */
#define LINE_TITLE 0
#define LINE_ROW(row) (1 + (row))

static int rows_shown(const TrainingMenu* menu)
{
    return menu->count < MENU_ROWS ? menu->count : MENU_ROWS;
}

static int line_description(const TrainingMenu* menu)
{
    return LINE_ROW(rows_shown(menu));
}

/// The nearest enabled item from `from` in direction `step` (1 or -1),
/// `from` included, or -1 if there is none.
static int enabled_from(const TrainingMenu* menu, int from, int step)
{
    int i;

    for (i = from; i >= 0 && i < menu->count; i += step) {
        if (menu->items[i].enabled) {
            return i;
        }
    }
    return -1;
}

/// Keeps the cursor on an enabled item and inside the row window.
static void settle_cursor(TrainingMenu* menu)
{
    int i;

    if (menu->cursor < 0 || menu->cursor >= menu->count) {
        menu->cursor = 0;
    }
    if (!menu->items[menu->cursor].enabled) {
        i = enabled_from(menu, menu->cursor, 1);
        if (i < 0) {
            i = enabled_from(menu, menu->cursor, -1);
        }
        if (i >= 0) {
            menu->cursor = i;
        }
    }
    if (menu->cursor < menu->top) {
        menu->top = menu->cursor;
    } else if (menu->cursor >= menu->top + MENU_ROWS) {
        menu->top = menu->cursor - MENU_ROWS + 1;
    }
    if (menu->top > menu->count - rows_shown(menu)) {
        menu->top = menu->count - rows_shown(menu);
    }
    if (menu->top < 0) {
        menu->top = 0;
    }
}

static void build_text(TrainingMenu* menu)
{
    HSD_Text* text = training_text_create();
    float x = menu->x != 0.0F ? menu->x : MENU_X;
    float top = menu->y != 0.0F ? menu->y : MENU_TITLE_Y;
    float rows_y = top + (MENU_ROW_Y - MENU_TITLE_Y);
    int row;

    training_text_add(text, x, top, MENU_TITLE_SCALE,
                      MENU_TITLE_SCALE, menu->title);
    for (row = 0; row < rows_shown(menu); row++) {
        training_text_add(text, x, rows_y + MENU_ROW_HEIGHT * row,
                          MENU_ROW_SCALE, MENU_ROW_SCALE, " ");
    }
    training_text_add(text, x,
                      rows_y + MENU_ROW_HEIGHT * rows_shown(menu) +
                          MENU_DESCRIPTION_GAP,
                      MENU_DESCRIPTION_SCALE, MENU_DESCRIPTION_SCALE, " ");
    menu->text = text;
}

static void redraw(TrainingMenu* menu)
{
    const TrainingMenuItem* current = &menu->items[menu->cursor];
    int row;

    for (row = 0; row < rows_shown(menu); row++) {
        int i = menu->top + row;
        const TrainingMenuItem* item = &menu->items[i];

        training_text_set(menu->text, LINE_ROW(row), item->label);
        training_text_set_colour(menu->text, LINE_ROW(row),
                                 !item->enabled      ? colour_disabled
                                 : i == menu->cursor ? colour_highlight
                                                     : colour_normal);
    }
    training_text_set(menu->text, line_description(menu),
                      current->description != NULL ? current->description
                                                   : " ");
}

void training_menu_show(TrainingMenu* menu)
{
    if (menu->count <= 0) {
        return;
    }
    if (menu->text == NULL) {
        build_text(menu);
    }
    settle_cursor(menu);
    redraw(menu);
    training_text_set_visible(menu->text, true);
}

void training_menu_hide(TrainingMenu* menu)
{
    if (menu->text != NULL) {
        training_text_set_visible(menu->text, false);
    }
}

void training_menu_reset(TrainingMenu* menu)
{
    menu->text = NULL;
    menu->cursor = 0;
    menu->top = 0;
}

#ifdef AUTHORING_BUILD
#define LOG_MENU(menu, item)                                                  \
    OSReport("[rep] event=menu menu=\"%s\" item=\"%s\"\n", (menu)->title,     \
             (item)->label)
#define LOG_MENU_BACK(menu)                                                   \
    OSReport("[rep] event=menu_back menu=\"%s\"\n", (menu)->title)
#else
#define LOG_MENU(menu, item) ((void) 0)
#define LOG_MENU_BACK(menu) ((void) 0)
#endif

/// Moves the cursor to the next enabled item in direction `step`, if any.
static void move(TrainingMenu* menu, int step)
{
    int i = enabled_from(menu, menu->cursor + step, step);

    if (i < 0) {
        return;
    }
    menu->cursor = i;
    settle_cursor(menu);
    lbAudioAx_80024030(SOUND_MOVE);
    redraw(menu);
}

TrainingMenuAction training_menu_update(TrainingMenu* menu, int port)
{
    u64 pressed = gm_GetButtonsTriggered((u8) port);
    u64 rapid = gm_801A36C0((u8) port);

    if (menu->text == NULL || menu->count <= 0) {
        return TrainingMenuAction_None;
    }
    if (pressed & PAD_BUTTON_START) {
        return TrainingMenuAction_Close;
    }
    if (pressed & PAD_BUTTON_B) {
        lbAudioAx_80024030(SOUND_BACK);
        LOG_MENU_BACK(menu);
        return TrainingMenuAction_Back;
    }
    if (pressed & PAD_BUTTON_A) {
        TrainingMenuItem* item = &menu->items[menu->cursor];

        if (item->enabled) {
            lbAudioAx_80024030(SOUND_CONFIRM);
            LOG_MENU(menu, item);
            if (item->confirm != NULL) {
                item->confirm();
            }
            if (menu->text != NULL) {
                settle_cursor(menu);
                redraw(menu);
            }
        }
        return TrainingMenuAction_None;
    }
    if (rapid & (PAD_ANY_UP | PAD_BUTTON_UP | PAD_STICK_UP)) {
        move(menu, -1);
    } else if (rapid & (PAD_ANY_DOWN | PAD_BUTTON_DOWN | PAD_STICK_DOWN)) {
        move(menu, 1);
    }
    return TrainingMenuAction_None;
}

#endif
