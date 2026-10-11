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
#define DRILL_STAGE_FOUNTAIN_OF_DREAMS 0x02
#define DRILL_STAGE_POKEMON_STADIUM 0x03
#define DRILL_STAGE_YOSHIS_STORY 0x08
#define DRILL_STAGE_DREAM_LAND 0x1C
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

/// The drill the rep loop runs: the session copy of the table entry chosen
/// with training_drill_select (at first the first entry).
const Drill* training_drill_current(void);

/// The same session copy, writable: the layout editor edits it. It is copied
/// from the table when a different drill is selected, so edits last until
/// then (restarts and retries keep them); the compiled table is never
/// modified.
Drill* training_drill_session(void);

/// Puts the compiled table entry's layout (landing point, drop heights,
/// player start) back into the session copy, discarding the edits.
void training_drill_revert_layout(void);

/// True when `a` and `b` have the same layout (landing point, drop heights,
/// player start); their other fields are not compared.
bool training_drill_same_layout(const Drill* a, const Drill* b);

/* Edited layout values are kept in tenths of a game unit, the precision
 * Print layout writes, so a printed layout pasted into the table is exactly
 * the layout that was tried. */

/// `value` rounded to the nearest tenth.
float training_drill_round_tenths(float value);

/// Writes `value` rounded to tenths with one decimal ("-12.3") to `out`, and
/// returns the end (the terminating 0).
char* training_drill_format_tenths(char* out, float value);

#ifdef AUTHORING_BUILD
/// Print layout: writes the current drill's (session copy's) layout fields,
/// landing_x to start_dy in the order of struct Drill, to the rep log as C
/// initializer text, ready to paste into the table in drill.c:
/// `[rep] layout: 60.0F, 0.0F, DrillHeight_List, 2, { 40.0F, 70.0F }, 0.0F, 0.0F,`
void training_drill_print_layout(void);
#endif

/// True when the drill has a pass mark (a pass_mark of 0 means none).
bool training_drill_has_pass_mark(const Drill* drill);

/// The table index of the current drill.
int training_drill_current_index(void);

/// The number of drills in the table.
int training_drill_count(void);

/// True when the current drill is not the last one in the table.
bool training_drill_has_next(void);

/// The number of rows the Event Match list shows: the table's drills, then
/// the Custom drill.
int training_drill_event_count(void);

/// The table index of the Custom drill: one past the compiled table.
int training_drill_custom_index(void);

/// True when `index` is the Custom drill's.
bool training_drill_is_custom(int index);

/// Sets the stage the Custom drill runs on (the stage select's pick).
void training_drill_custom_set_stage(int stage);

/// While the Custom drill is current, makes its layout fit the stage in play,
/// once per stage (otherwise does nothing): the
/// first time, or when the stored landing point has no floor on this stage,
/// the layout goes back to the defaults (landing point at the middle of the
/// main floor, found with a floor ray-cast at the stage centre). A stored
/// layout with a floor is kept. Needs the stage's collision, so it does
/// nothing (and tries again next time) when the cast finds no floor at all.
void training_drill_custom_fit(void);

/// The Custom drill's session record, writable: the settings menu edits its
/// fields. It lasts until power-off.
Drill* training_drill_custom(void);

/// The table entry at `index`; the Custom drill's session record at
/// training_drill_custom_index().
const Drill* training_drill_at(int index);

/// Makes the table entry at `index` (or the Custom drill) the current drill
/// (ignored when out of range). A drill event selects its own drill this way.
void training_drill_select(int index);

/// A drop height for one rep: the only height, one of a list at random, or a
/// uniform pick from a range (game units above the landing surface).
float training_drill_pick_height(const Drill* drill);

#endif

#endif
