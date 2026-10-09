#ifndef TRAINING_SCORE_H
#define TRAINING_SCORE_H

#ifdef TRAINING_BUILD

#include <stdbool.h>

#include "drill.h"
#include "rep_result.h"

/* The counted reps in the current drill run in which the opponent did one
 * tech (what it did, not what the drill aimed for): how many, how many
 * succeeded, and how many failures were early or late. */
typedef struct TechScore {
    int counted;
    int successes;
    int early;
    int late;
} TechScore;

/* Score (score.c): reset on entering Training, and fed each counted rep. */
void training_score_init(void);
void training_score_show(void);
void training_score_record(RepTech tech, bool success);
/* Records a counted rep's final timing class, once it can no longer change
 * (a failure can turn late during the pause). Only early and late are kept. */
void training_score_record_timing(RepTech tech, RepTimingKind kind);
TechScore training_score_tech(RepTech tech);
void training_score_clear(void);
int training_score_successes(void);
int training_score_counted(void);
/* True when the current score has reached the drill's pass mark. */
bool training_score_passed(const Drill* drill);

/* End-of-drill summary (score.c): shown when the drill's length is reached. */
void training_summary_show(const Drill* drill);
void training_summary_hide(void);

#endif

#endif
