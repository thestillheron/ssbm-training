#ifndef TRAINING_LAYOUT_EDITOR_H
#define TRAINING_LAYOUT_EDITOR_H

#ifdef TRAINING_BUILD

#include <stdbool.h>

/* The layout editor (layout_editor.c): Edit layout in the pause menu opens it
 * over the frozen match. It edits the current drill's session copy
 * (training_drill_session, drill.h), never the compiled table, and shows the
 * layout with the fighters as markers (tech_chase.c's edit mode): the
 * opponent hangs at the selected drop height above the landing point, the
 * player stands at its start. The stick moves the opponent marker: left and
 * right the landing x, snapped to the floor below (a move that would leave no
 * floor below the landing point or the player's start is clamped to the last
 * valid position); up and down its height, smoothly: the landing surface and
 * the drop height follow from where it is, so passing a platform at the
 * landing x makes that the landing surface with no jump. The C-stick moves
 * the player start: its x moves along the surface, and a flick of its y steps
 * the player onto the next surface above or below (start_dy is set to
 * resolve onto it), so the floor or a platform is a deliberate choice; the
 * values text says which the player stands on. Holding Z or a trigger makes
 * the sticks and the drop height adjustment fine.
 * Edited values are kept in tenths (drill.h). The current values (landing
 * x, drop heights, player start offset) are shown as text and update live;
 * A reverts to the compiled drill's layout. B or Start leaves: if the layout
 * changed, the drill restarts with it; otherwise the drill carries on. */

/// Opens the editor (the pause menu is already hidden) and places the
/// markers.
void training_layout_editor_open(void);

/// Handles one frame of `port`'s input while open. False once B or Start has
/// closed it (edit mode is over, and the drill restarted if the layout
/// changed); the caller then unfreezes the match.
bool training_layout_editor_update(int port);

/// True while the editor is open. The pause camera's own use of the sticks
/// (zoom and pan, which the C-stick's up and down would also drive) is off
/// meanwhile.
bool training_layout_editor_is_open(void);

/// At a drill event's match start: forgets the previous scene's text.
void training_layout_editor_init(void);

#endif

#endif
