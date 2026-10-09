#ifndef TRAINING_PUNISH_WINDOW_H
#define TRAINING_PUNISH_WINDOW_H

#ifdef TRAINING_BUILD

#include "rep_result.h"

/* Punish window lengths measured this session (punish_window.c), per
 * opponent character and per tech: the frames from the landing to the
 * actionable frame, as the last rep that measured it found them. Kept until
 * the game is reset, so the pause menu can show them for the opponent in
 * play. */

/* Records the window length of a rep against `opponent_kind` (FighterKind)
 * in which the opponent did `tech`. */
void training_punish_window_record(int opponent_kind, RepTech tech,
                                   int frames);

/* The last recorded length, or NO_FRAME before any rep measured it. */
int training_punish_window(int opponent_kind, RepTech tech);

/* The FighterKind of the opponent in play, or -1 before it exists. */
int training_punish_window_opponent_kind(void);

/* The panel: a heading and one line per tech in the current drill's pool,
 * each with its measured length for the opponent in play, or "--" until a
 * rep has measured it. Shown by the pause menu and at the start of a drill. */

/* Shows the panel, building its text the first time (once the scene has
 * settled) and refreshing the lengths. */
void training_punish_window_panel_show(void);

/* Hides the panel; a NULL-safe no-op before it was ever shown. */
void training_punish_window_panel_hide(void);

/* Forgets the panel's text. Call at each match start: the previous scene's
 * text is gone. */
void training_punish_window_panel_reset(void);

/* The panel's lines after the heading, and the tech on line `line` (0 to
 * lines - 1). Valid once the panel has been shown. */
int training_punish_window_panel_lines(void);
RepTech training_punish_window_panel_tech(int line);

#endif

#endif
