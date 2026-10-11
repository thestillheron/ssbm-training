#ifndef TRAINING_EVENT_H
#define TRAINING_EVENT_H

#ifdef TRAINING_BUILD

#include <stdbool.h>

#include <melee/gm/forward.h>
#include <melee/mn/forward.h>

/* Drills as Event Match events (ADR-0005). The Event Match game mode
 * (src/melee/gm/gmevent.c) calls these from one-line TRAINING_BUILD hooks.
 * Each returns false (and changes nothing) when the event in play is not a
 * drill event, so the retail code runs on; true means the event is ours and
 * the hook's retail code must be skipped. */

/// The drill table index of the event in play, or -1 for a vanilla event.
int training_event_drill_index(void);

/// True while a drill event is in play (the Event Match game mode, on a
/// drill's row of the list).
bool training_event_drill_in_play(void);

/// True while a drill event is in play. Fighter_procInput reads the human's
/// C-stick in it: retail zeroes the C-stick in every 1-P mode, Event Match
/// included, and drills need smash and aerial attacks on it.
bool training_event_cstick_enabled(void);

/// From gm_Mode_Event_OnLoad: a drill event always opens on the character
/// select (retail skips it for events with a fixed character).
void training_event_on_load(void);

/// From onEnterCss: Training Mode's player + CPU character select (type 0x17)
/// with the characters last picked for a drill event.
bool training_event_enter_css(CSSData* css);

/// From onExitCss, after retail's "back to the event list" check: keeps the
/// player's and the CPU's picks.
bool training_event_exit_css(CSSData* css);

/// The Custom drill's stage select state (gm_Mode_Event_States): its scene
/// data and enter/exit hooks. Exiting with a stage sets the Custom drill's
/// stage and goes to the match; B goes back to the character select.
extern SSSData training_event_sss_data;
void training_event_enter_sss(GameModeState* state);
void training_event_exit_sss(GameModeState* state);

/// From onEnterVs: the match setup built from the drill (its stage, the two
/// fighters from the character select, an endless match) in place of the
/// event data file; the drill starts when the match does.
bool training_event_enter_vs(StartMeleeData* md);

/// Where ending a drill match from the pause menu goes next.
typedef enum TrainingEventExit {
    TrainingEventExit_CharacterSelect, /* the CSS for the same event */
    TrainingEventExit_EventList,       /* the list, on the drill just played */
    TrainingEventExit_NextDrill, /* the next drill, same fighters */
} TrainingEventExit;

/// Ends the drill match (the match must not be frozen) and sends onExitVs to
/// `exit`.
void training_event_end_match(TrainingEventExit exit);

/// From onExitVs: a retry reloads the match, a match ended by L+R+A+Start or
/// from the pause menu goes to the character select or the event list as
/// chosen, and anything else goes back to the event list. Never writes Event Match's scores or cleared bits.
bool training_event_exit_vs(MatchExitInfo* exit);

#endif

#endif
