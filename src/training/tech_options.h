#ifndef TRAINING_TECH_OPTIONS_H
#define TRAINING_TECH_OPTIONS_H

#ifdef TRAINING_BUILD

/* The opponent's tech options, named relative to the player (see the glossary:
 * in is toward the player, away is away from the player). */
typedef enum TechOption {
    TechOption_InPlace,
    TechOption_In,
    TechOption_Away,
    TechOption_Miss,
    TechOption_Count,
} TechOption;

#define TECH_OPTION_BIT(option) (1 << (option))

/* The option pool and the missed-tech down time: the one place they are set.
 * Drill-agnostic defaults for now; drills-as-data will supply them per drill.
 * Each rep picks uniformly from the pool. */
#ifndef TECH_OPTION_POOL
#define TECH_OPTION_POOL TECH_OPTION_BIT(TechOption_InPlace)
#endif

/* Frames, counted from the landing, before the plain getup input is fed after
 * a missed tech: 45 (0.75 s) is a beat in which the opponent visibly lies
 * down, as players do before getting up, and far shorter than the game's own
 * auto-getup timer (over 270 frames, measured). */
#define MISSED_TECH_DOWN_TIME 45

/* The option chosen for the current rep. It is chosen before the drop, so
 * features that run later in the rep can read it. */
TechOption training_tech_option(void);

#endif

#endif
