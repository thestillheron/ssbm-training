#ifndef TRAINING_CUSTOM_SETTINGS_H
#define TRAINING_CUSTOM_SETTINGS_H

#ifdef TRAINING_BUILD

#include <stdbool.h>

#include <melee/mn/mnevent.h>
#include <sysdolphin/baselib/gobj.h>

/* The Custom drill's settings menu (custom_settings.c): choosing the Custom
 * drill on the Event Match list opens it over the list, built with the menu
 * component (menu.h). Each row edits a field of the Custom drill's
 * session-scoped record (training_drill_custom), so settings last until
 * power-off. The Start row goes on to the character select as choosing any
 * other event does; B closes the menu and keeps the list.
 *
 * Adding a row: write its value and activate functions in custom_settings.c
 * and add one line to the `rows` table there. */

/// Called every frame from the event select screen (mnEvent_8024D864) with the
/// screen's gobj (NULL before it exists: then does nothing and returns false)
/// and the frame's menu inputs, before the screen handles them. Opens the menu when
/// the Custom drill is chosen, and while it is open handles all the input
/// and returns true (the screen must do nothing else). When Start is chosen
/// it closes the menu, sets *inputs to a plain confirm so the screen carries
/// on to the character select as for any event, and returns false.
bool training_custom_settings_input(HSD_GObj* gobj, u64* inputs);

#endif

#endif
