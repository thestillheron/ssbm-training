#ifndef TRAINING_NEW_SAVE_H
#define TRAINING_NEW_SAVE_H

#ifdef TRAINING_BUILD

/// Unlocks every character and stage in the save data about to be created.
void training_new_save_unlock_all(void);

/// The comment title of the training build's save.
const char* training_new_save_title(void);

#endif

#endif
