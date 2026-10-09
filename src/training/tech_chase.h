#ifndef TRAINING_TECH_CHASE_H
#define TRAINING_TECH_CHASE_H

#ifdef TRAINING_BUILD

#include "layout.h"

/* The tech chase rep loop (tech_chase.c), which runs every drill. */

/// Called at a drill event's match start (event.c): starts the rep loop.
void training_tech_chase_init(void);

/// Restarts the drill in play from rep 1 (the pause menu's Restart drill).
void training_tech_chase_restart(void);

/* Edit mode (the layout editor, layout_editor.c), over the frozen match: the
 * fighters are markers for the current drill's layout and the opponent's fed
 * input is neutral. */

/// Enters edit mode.
void training_tech_chase_edit_begin(void);

/// Places the markers for the current drill's layout: the opponent hangs
/// `heights[height_index]` above the landing point, the player stands at its
/// start, and the resolved layout goes in `out` (if not NULL). On a status
/// other than Layout_Ok nothing is moved and `out` is unchanged.
LayoutStatus training_tech_chase_edit_place(int height_index, Layout* out);

/// Leaves edit mode. If `layout_changed`, restarts the drill from rep 1 with
/// the layout as edited; otherwise the drill carries on (reps and score
/// kept), dropping the rep the markers displaced if one was under way.
void training_tech_chase_edit_end(bool layout_changed);

/// Writes a rep log line `event` with the rep loop's progress, then `extra`.
/// Authoring builds only; a no-op otherwise.
void training_tech_chase_log_progress(const char* event, const char* extra);

#endif

#endif
