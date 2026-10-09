#ifndef TRAINING_EVENT_LIST_H
#define TRAINING_EVENT_LIST_H

#ifdef TRAINING_BUILD

#include <melee/mn/mnevent.h>

/* The Event Match list (the retail event select screen, src/melee/mn/
 * mnevent.c) showing the drill table in place of the vanilla events
 * (ADR-0005): one row per drill, in table order, numbered from 1. A row's
 * position on the list is its drill's index in the table, and is what the
 * screen stores as the chosen event (gmMainLib_804D3EE0->vs.unk_530.unk_535).
 * The screen calls these from one-line TRAINING_BUILD hooks; in the training
 * build the list always shows the drills. */

/// The list's last possible top row: the screen shows 9 rows and scrolls
/// until the last drill is on the bottom one (0 when every drill fits).
int training_event_list_last_top(void);

/// Builds the row on screen line `line` (0 to 8) for list position `event`:
/// the drill's number and name, or an empty line past the last drill.
/// `pos` is the line's position, as the screen computes it.
void training_event_list_row(MnEventData* data, int line, int event,
                             const Vec3* pos);

/// Shows the highlighted drill's description and pass mark in the
/// description box (the retail event's portrait, score and score kind are
/// hidden).
void training_event_list_show(HSD_GObj* gobj);

/// True when the highlighted row is the last drill, so the cursor can't
/// move further down.
bool training_event_list_at_end(const MnEventData* data);

/// Shows or hides the screen's rows and description (the Custom drill's
/// settings menu is drawn over the list).
void training_event_list_set_visible(MnEventData* data, bool visible);

#endif

#endif
