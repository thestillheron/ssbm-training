#ifndef TRAINING_CUSTOM_SETTINGS_H
#define TRAINING_CUSTOM_SETTINGS_H

#ifdef TRAINING_BUILD

#include <stdbool.h>

#include <melee/mn/mnevent.h>
#include <sysdolphin/baselib/gobj.h>

/* The Custom Drill screen (custom_settings.c): the event scene opened on the
 * Custom drill's index, which is past the list's last row, shown as the
 * settings menu on the menu background with the list hidden. The scene opens
 * that way from the main menu's Custom Drill, and again when a custom drill
 * or its character select returns to the menu (the stored event is still the
 * Custom drill), so "Settings" in the pause and summary menus and B on the
 * character select all land here. Built with the menu component (menu.h);
 * each row edits a field of the Custom drill's session-scoped record
 * (training_drill_custom), so settings last until power-off. Start goes on to
 * the character select; B returns to the main menu on Custom Drill.
 *
 * Adding a row: write its value and activate functions in custom_settings.c
 * and add one line to the `rows` table there. */

/// Called every frame from the event select screen (mnEvent_8024D864) with the
/// screen's gobj (NULL before it exists: then does nothing and returns false)
/// and the frame's menu inputs, before the screen handles them. While the
/// scene is on the Custom drill's index it opens the menu (hiding the list)
/// and handles all the input, returning true (the screen must do nothing
/// else); B goes to the main menu. When Start is chosen it closes the menu,
/// sets *inputs to a plain confirm so the screen carries on to the character
/// select with the Custom drill as the chosen event, and returns false.
bool training_custom_settings_input(HSD_GObj* gobj, u64* inputs);

#endif

#endif
