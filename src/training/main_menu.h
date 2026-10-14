#ifndef TRAINING_MAIN_MENU_H
#define TRAINING_MAIN_MENU_H

#ifdef TRAINING_BUILD

#include <stdbool.h>

#include <melee/mn/forward.h>
#include <sysdolphin/baselib/gobj.h>

/* The training build's main menu (main_menu.c, ADR-0007): retail's top level
 * (src/melee/mn/mnmain.c) with four selections, RTC 101, Custom Drill,
 * Options and Data. They are retail's own selections, so the menu lays them
 * out, tints and highlights them as retail does:
 *
 *   SEL_MAIN_1P       RTC 101       (retail 1-P Mode's label frame)
 *   SEL_MAIN_VS       Custom Drill  (retail VS. Mode's)
 *   SEL_MAIN_SETTINGS Options
 *   SEL_MAIN_DATA     Data
 *
 * SEL_MAIN_TOY (Trophies) is locked, so the cursor skips it and the menu
 * draws the four with no gap. The screen calls these from one-line
 * TRAINING_BUILD hooks. */

/// Whether `selection` of the main menu can be chosen (mn_80229938's answer
/// for MENU_KIND_MAIN): every selection but Trophies.
bool training_main_menu_selectable(int selection);

/// Called when A is pressed on the main menu (mn_8022DB10) with its input
/// gobj. Logs the choice and, for RTC 101 and Custom Drill, opens the event
/// scene (on its first drill for RTC 101; on the Custom drill's index for
/// Custom Drill, which custom_settings.c shows as a screen of its own) and
/// returns true: the screen must do nothing else. Returns false for Options
/// and Data, which the screen opens as retail does.
bool training_main_menu_confirm(HSD_GObj* gp);

/// Chooses the top-level image of the heading strip that the event scene
/// shows. The strip's animation reads frame 0 of its image table there:
/// "RTC 101" for the event list, and for the Custom Drill screen this points
/// frame 0 at the table's "Custom Drill" image (tools/archive_patches.py
/// patches both; the second level is blank in both). Call it with whether
/// the scene is the Custom Drill screen whenever one opens.
void training_main_menu_heading(bool custom_drill);

#endif

#endif
