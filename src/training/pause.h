#ifndef TRAINING_PAUSE_H
#define TRAINING_PAUSE_H

#ifdef TRAINING_BUILD

/* The drill pause menu (pause.c). In a drill event the retail pause is off
 * (rules.disable_pausing); instead Start on any human port freezes the match
 * with the game's own pause flag, sound and HUD calls, and opens our menu,
 * driven by that port. Resume or Start unfreezes it; B never does. The menu
 * also shows the opponent's punish window for the drill's techs, or "--"
 * until a rep has measured it. In authoring builds the rep log gets `pause`
 * and `resume` lines with the rep loop's progress. */

/* At a drill event's match start: sets up the pause handling for the
 * match. */
void training_pause_init(void);

#endif

#endif
