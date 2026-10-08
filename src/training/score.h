#ifndef TRAINING_SCORE_H
#define TRAINING_SCORE_H

#ifdef TRAINING_BUILD

#include <sysdolphin/baselib/sislib.h>

#include <stdbool.h>

/* Score (score.c): reset on entering Training, and fed each counted rep. */
void training_score_init(void);
void training_score_show(void);
void training_score_record(bool success);

/* Creates an empty text object for training on-screen text. */
HSD_Text* training_text_create(void);

#endif

#endif
