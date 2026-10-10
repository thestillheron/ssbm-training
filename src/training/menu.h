#ifndef TRAINING_MENU_H
#define TRAINING_MENU_H

#ifdef TRAINING_BUILD

#include <stdbool.h>

#include <sysdolphin/baselib/forward.h>

#include "text.h"

/* A text menu (menu.c): a title, a list of items in a 9-row window that
 * scrolls, and the highlighted item's description. Used by the drill pause
 * menu and the drill summary.
 *
 * The owner keeps a TrainingMenu (usually static), fills in title, items and
 * count, shows it, then calls training_menu_update once a frame with the
 * port that drives it until it hides it. The menu reads that port's
 * rapid-held input: up/down (d-pad or stick) moves the cursor, skipping
 * disabled items, A runs the highlighted item's confirm callback, B and
 * Start are handed back to the owner (B goes back, Start closes; neither
 * hides the menu by itself). It plays the game's menu sounds (2 move,
 * 1 confirm, 0 back), shows the highlight by text colour and greys disabled
 * items. In authoring builds each confirm writes a `menu` line to the rep
 * log. */

/* lbAudioAx_80024030's menu back sound, which the menu plays on B; for
 * screens that leave the same way. */
#define TRAINING_MENU_SOUND_BACK 0

typedef struct TrainingMenuItem {
    const char* label;
    /* One line, shown while the item is highlighted; NULL for none. */
    const char* description;
    bool enabled;
    /* Run when A is pressed on the item. */
    void (*confirm)(void);
} TrainingMenuItem;

typedef enum TrainingMenuAction {
    TrainingMenuAction_None,
    TrainingMenuAction_Back,  /* B was pressed */
    TrainingMenuAction_Close, /* Start was pressed */
} TrainingMenuAction;

typedef struct TrainingMenu {
    const char* title;
    TrainingMenuItem* items;
    int count;
    /* Set by the menu. */
    int cursor;
    int top;
    HSD_Text* text;
    /* Left edge and title line in canvas units; 0 for the default. Set by
     * the owner, after the fields above. */
    float x;
    float y;
    /* Set by the menu: the panel behind the text. */
    TrainingPanel panel;
} TrainingMenu;

/* Shows the menu, building its text the first time (once the scene has
 * settled) and redrawing it from the items, which may have changed. The
 * cursor stays where it was, moved to an enabled item if needed. */
void training_menu_show(TrainingMenu* menu);

/* Hides the menu; showing it again keeps the cursor. */
void training_menu_hide(TrainingMenu* menu);

/* Handles one frame of `port`'s input (see above). Call only while shown. */
TrainingMenuAction training_menu_update(TrainingMenu* menu, int port);

/* Frees the menu's text and panel and resets it as training_menu_reset does,
 * for a menu closed while its scene goes on. */
void training_menu_free(TrainingMenu* menu);

/* Forgets the menu's text and puts the cursor on the first item. Call at
 * each scene start: the previous scene's text is gone. */
void training_menu_reset(TrainingMenu* menu);

#endif

#endif
