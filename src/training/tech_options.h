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

/* The option pool is per drill (see drill.h): each rep picks uniformly from it. */

/* Frames, counted from the landing, before the plain getup input is fed after
 * a missed tech: 45 (0.75 s) is a beat in which the opponent visibly lies
 * down, as players do before getting up, and far shorter than the game's own
 * auto-getup timer (over 270 frames, measured). */
#define MISSED_TECH_DOWN_TIME 45

/* A missed tech's punish window: the opponent cannot choose its getup until
 * frame 26 (the landing frame is frame 1), so a hit or grab on frames 1..25
 * is in time and one on frame 26 or later is late. Frame 26 closes the window
 * as the first actionable frame does for a tech; the opponent keeps lying
 * until MISSED_TECH_DOWN_TIME, so a late hit still connects. */
#define MISSED_TECH_WINDOW 25

/* The option chosen for the current rep. It is chosen before the drop, so
 * features that run later in the rep can read it. */
TechOption training_tech_option(void);

#endif

#endif
