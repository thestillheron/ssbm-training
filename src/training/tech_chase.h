#ifndef TRAINING_TECH_CHASE_H
#define TRAINING_TECH_CHASE_H

#ifdef TRAINING_BUILD

/* The tech chase rep loop (tech_chase.c), which runs every drill. */

/// Called at a drill event's match start (event.c): starts the rep loop.
void training_tech_chase_init(void);

/// Restarts the drill in play from rep 1 (the pause menu's Restart drill).
void training_tech_chase_restart(void);

/// Writes a rep log line `event` with the rep loop's progress, then `extra`.
/// Authoring builds only; a no-op otherwise.
void training_tech_chase_log_progress(const char* event, const char* extra);

#endif

#endif
