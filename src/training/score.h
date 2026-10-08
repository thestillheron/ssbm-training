#ifndef TRAINING_SCORE_H
#define TRAINING_SCORE_H

#ifdef TRAINING_BUILD

#include <sysdolphin/baselib/sislib.h>

#include <stdbool.h>

#include "drill.h"
#include "tech_options.h"

/* Score (score.c): reset on entering Training, and fed each counted rep. */
void training_score_init(void);
void training_score_show(void);
void training_score_record(TechOption option, bool success);
void training_score_clear(void);
int training_score_successes(void);
int training_score_counted(void);
/* True when the current score has reached the drill's pass mark. */
bool training_score_passed(const Drill* drill);

/* End-of-drill summary (score.c): shown when the drill's length is reached. */
void training_summary_show(const Drill* drill);
void training_summary_hide(void);

/* Creates an empty text object for training on-screen text. */
HSD_Text* training_text_create(void);

#endif

#endif
