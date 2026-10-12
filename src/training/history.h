#ifndef TRAINING_HISTORY_H
#define TRAINING_HISTORY_H

#ifdef TRAINING_BUILD

#include <stdbool.h>

#include <melee/mn/mnevent.h>
#include <training/drill.h>
#include <sysdolphin/baselib/gobj.h>



/* Drill history (history.c; glossary: history, attempt, history graph): the
 * score of every recorded attempt at a drill, keyed by the drill's stable ID
 * (DrillId), kept in RAM, and X on a drill in the event list to see them on a
 * graph. */

/// Most scores kept per drill: the latest ones. The count of all attempts is
/// kept besides, so the graph's x axis shows real attempt numbers.
#define HISTORY_KEEP 100

/// Records an attempt at the current drill with the current score, when the
/// drill keeps a history and its layout is the table's. Call when the drill
/// summary opens, and only then: an attempt left early records nothing.
/// Returns true when it recorded. An edited layout, the Custom drill and
/// drills without a history are not recorded.
bool training_history_record(void);

/// How many attempts at the drill have been recorded in all.
int training_history_total(DrillId id);

/// How many scores are kept for the drill: the latest min(total, HISTORY_KEEP).
int training_history_count(DrillId id);

/// The `index`th kept score of the drill, oldest first (0 to count - 1).
int training_history_score(DrillId id, int index);

/// Called every frame from the event select screen (mnEvent_8024D864) with the
/// screen's gobj (NULL before it exists) and the frame's menu inputs, after
/// the Custom drill's settings menu. X on a drill with a history opens its
/// graph over the list; while it is open it handles all input (B or X closes
/// it) and returns true. X on any other row does nothing and returns true.
/// Otherwise returns false.
bool training_history_input(HSD_GObj* gobj, u64* inputs);

#endif

#endif
