#ifndef TRAINING_REP_RESULT_H
#define TRAINING_REP_RESULT_H

#ifdef TRAINING_BUILD

#include <stdbool.h>

#include "tech_options.h"

/* Rep result (rep_result.c): how a rep ended and its timing, classified from
 * the frames the rep loop measured, written to the rep log (authoring builds)
 * and shown on screen during the pause after it. See the glossary's rep
 * result and timing. */

/* Expands to its arguments in authoring builds only: for rep log text in a
 * table that player builds also use (the rep log is authoring-only). */
#ifdef AUTHORING_BUILD
#define REP_LOG_ONLY(...) __VA_ARGS__
#else
#define REP_LOG_ONLY(...)
#endif

/* Frame number meaning "has not happened (yet) in this rep". */
#define NO_FRAME (-1)

/* How the rep was scored. A late hit is still a failure. */
typedef enum RepOutcome {
    RepOutcome_Success,
    RepOutcome_Failure,
    RepOutcome_Void,
    RepOutcome_Count,
} RepOutcome;

/* What the opponent did on landing, as the result and the summary name it.
 * The first four are the tech options, in TechOption's order. */
typedef enum RepTech {
    RepTech_InPlace = TechOption_InPlace,
    RepTech_In = TechOption_In,
    RepTech_Away = TechOption_Away,
    RepTech_Missed = TechOption_Miss,
    RepTech_Wall,
    RepTech_Ceiling,
    RepTech_Count,
} RepTech;

/* How the player's punish reached the opponent. */
typedef enum HitKind {
    HitKind_None,
    HitKind_Hit,
    HitKind_Grab,
} HitKind;

/* One hit or grab of the player's on the opponent: the rep frame it connected
 * on (NO_FRAME for none) and how. */
typedef struct PlayerHit {
    int frame;
    HitKind kind;
} PlayerHit;

/* The frames of one rep as the rep loop measured them, counted from the drop.
 * The punish window opens on the landing frame and closes on the first
 * actionable frame, which is not part of it: the vulnerable frames are the
 * window frames before it in which the opponent can be hit or grabbed. */
typedef struct RepFrames {
    int window_open;
    int vuln_from;
    int vuln_to;
    int actionable;
    /* The last vulnerable frame as predicted from the opponent's animation,
     * or NO_FRAME: a success ends the window at the hit, so this is where it
     * would have ended. */
    int predicted_vuln_to;
    /* The hit or grab inside the window (a success), or before the landing
     * (a void). */
    PlayerHit hit;
    /* After a failure: the first hit or grab from the actionable frame to the
     * end of the pause. */
    PlayerHit late_hit;
    /* The last window frame in which the player had a hitbox or grab box out
     * while the opponent was intangible, and how many frames before the next
     * vulnerable frame (or the actionable frame) it was; NO_FRAME if none. */
    int early_attack_frame;
    int early_frames;
    /* The rep was voided by a hit before the landing. */
    bool voided;
} RepFrames;

/* The timing class of a rep. To add one: a kind here, its row in
 * timing_texts (rep_result.c) and its case in training_rep_classify. */
typedef enum RepTimingKind {
    /* A failure with no attack out during the intangible part of the window. */
    RepTiming_None,
    /* The hit or grab landed on vulnerable frame k of n. */
    RepTiming_Success,
    /* A hit or grab before the opponent landed. */
    RepTiming_Void,
    /* A failure in which the player had a hitbox or grab box out while the
     * opponent was intangible: `frames` is how many frames before the next
     * vulnerable frame the last such frame was. */
    RepTiming_Early,
    /* A failure whose hit or grab landed `frames` after the opponent's first
     * actionable frame, during the pause. Takes precedence over early. */
    RepTiming_Late,
    RepTiming_Count,
} RepTimingKind;

typedef struct RepTiming {
    RepTimingKind kind;
    /* Success: the vulnerable frame hit (from 1) and how many there were. n
     * is 0 when it could not be known, and never less than k. */
    int k;
    int n;
    /* Frames early or late, for the classes that report them. */
    int frames;
} RepTiming;

/* The rep's timing class, from its measured frames. Final once the pause is
 * over (a failure can still turn late during it). */
RepTiming training_rep_classify(const RepFrames* frames);

/* What the opponent was going to do, for a rep that ended before it landed. */
RepTech training_rep_tech_of_option(TechOption option);

/* "Tech in place", "Tech in", ...: line 1 of the result. */
const char* training_rep_tech_label(RepTech tech);
/* "In place", "In", ...: the summary's option column. */
const char* training_rep_tech_short_label(RepTech tech);

#ifdef AUTHORING_BUILD
/* Longest `timing=` value, with the terminator. */
#define REP_TIMING_LOG_MAX 32

/* The rep log's `timing=` value: "success_k/n", "early_Nf", "late_Nf",
 * "none" or "void". */
void training_rep_timing_log_value(char* dst, const RepTiming* timing);
/* The rep log's name of a tech ("in-place", "in", "away", "miss", "wall",
 * "ceil") and of an outcome ("success", "failure", "void"). */
const char* training_rep_tech_log_name(RepTech tech);
const char* training_rep_outcome_log_name(RepOutcome outcome);
#endif

/* The on-screen result. Init it on entering Training, create it once the
 * scene has settled, show it when a rep ends and hide it at the next reset.
 * Line 1 is the tech and the outcome, coloured by the outcome; line 2 is the
 * timing in white: "Frame k/n", "Nf early", "Nf late", "No attack" or
 * "Early: hit before landing". */
void training_rep_result_init(void);
void training_rep_result_create(void);
void training_rep_result_show(RepTech tech, RepOutcome outcome,
                              const RepTiming* timing);
void training_rep_result_hide(void);

#endif

#endif
