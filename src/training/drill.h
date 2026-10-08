#ifndef TRAINING_DRILL_H
#define TRAINING_DRILL_H

#ifdef TRAINING_BUILD

#include <stdbool.h>

#include "tech_options.h"

/* A drill is a data record: the rep loop runs whichever drill is current and
 * reads its layout and settings from here (see the glossary: drill, layout).
 * The curriculum is the const table in drill.c. Heights are in game units
 * above the landing surface; the landing point is the stage-relative spot the
 * opponent drops over. */

#define DRILL_MAX_HEIGHTS 4

/* External stage ids. */
#define DRILL_STAGE_BATTLEFIELD 0x1F
#define DRILL_STAGE_FINAL_DESTINATION 0x20

typedef enum DrillHeightKind {
    DrillHeight_List, /* heights[0..count): one picked at random per rep */
    DrillHeight_Range, /* heights[0]..heights[1], picked uniformly */
} DrillHeightKind;

typedef enum DrillFacing {
    DrillFacing_TowardPlayer,
    DrillFacing_Away,
    DrillFacing_Left,
    DrillFacing_Right,
} DrillFacing;

typedef enum DrillLengthKind {
    DrillLength_Reps,
    DrillLength_Time, /* length in seconds */
} DrillLengthKind;

typedef struct Drill {
    const char* name;
    const char* description;
    int stage;
    /* Landing point: x, and a y hint choosing which surface to snap to. */
    float landing_x;
    float landing_y;
    DrillHeightKind height_kind;
    int height_count;
    float heights[DRILL_MAX_HEIGHTS];
    /* Player start. start_dx is from the adjacent point (the landing x moved
     * left by the two fighters' reach, so they stand side by side), not from
     * the landing x. start_dy is from the landing surface; on a platform
     * landing, 0 or less puts the player on the floor beneath it. */
    float start_dx;
    float start_dy;
    DrillFacing opponent_facing;
    bool reset_player;
    int option_pool; /* TECH_OPTION_BIT mask */
    bool vanish;
    DrillLengthKind length_kind;
    int length;
    int pass_mark;
} Drill;

/// The drill the rep loop runs: the table entry chosen with
/// training_drill_choose_next (at first the compiled-in index).
const Drill* training_drill_current(void);

/// The number of drills in the table.
int training_drill_count(void);

/// The table entry at `index`.
const Drill* training_drill_at(int index);

/// True when `drill` can run on `stage` (an external stage id): a curriculum
/// drill runs only on its named stage.
bool training_drill_fits_stage(const Drill* drill, int stage);

/// Makes the current drill the first one that fits `stage`, if the current
/// one does not. Leaves it alone when none fits.
void training_drill_fit_to_stage(int stage);

/// Moves the current drill to the next table entry that fits `stage`,
/// wrapping round. Returns false (and changes nothing) when no other drill
/// fits.
bool training_drill_choose_next(int stage);

/// A short name for an external stage id, for on-screen text.
const char* training_drill_stage_name(int stage);

/// A drop height for one rep: the only height, one of a list at random, or a
/// uniform pick from a range (game units above the landing surface).
float training_drill_pick_height(const Drill* drill);

#endif

#endif
