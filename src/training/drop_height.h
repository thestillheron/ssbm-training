#ifndef TRAINING_DROP_HEIGHT_H
#define TRAINING_DROP_HEIGHT_H

#ifdef TRAINING_BUILD

/* Character drop heights: the default height a drill drops the opponent from,
 * one per opponent character (see the glossary: character drop height). */

/// The fallback height, for a fighter kind without a row.
#define DROP_HEIGHT_FALLBACK 40.0F

/// The character drop height for `fighter_kind` (a FighterKind), or the
/// fallback for a kind without a row (or -1, before the opponent exists).
float training_drop_height(int fighter_kind);

#endif

#endif
