#ifndef TRAINING_LAYOUT_H
#define TRAINING_LAYOUT_H

#ifdef TRAINING_BUILD

#include "drill.h"

#include <stdbool.h>

#include <sysdolphin/baselib/mtx.h>

/* A drill's layout resolved against the stage in play: the landing point and
 * the player's start, each snapped onto the surface below it. */

typedef enum LayoutStatus {
    Layout_Ok,
    Layout_NoLandingFloor, /* nothing to stand on below the landing point */
    Layout_NoStartFloor, /* nothing to stand on below the player start */
    Layout_BadHeights, /* the drill has no usable drop heights */
} LayoutStatus;

typedef struct Layout {
    Vec3 landing; /* on the landing surface (floor or platform) */
    Vec3 start; /* on the floor below the player's start spot */
} Layout;

/// Snaps `drill`'s layout onto the current stage with a downward ray-cast of
/// the stage collision's floor check. The landing point is cast from its y
/// hint, so the hint chooses between the main floor and a platform. The
/// player's start x is the landing x moved `start_gap` to the left (the
/// distance that stands the player beside the opponent, from the fighters'
/// reach) plus the drill's start_dx, so start_dx is relative to that adjacent
/// point, not to the landing x. Its floor is cast down from the landing
/// surface's height plus start_dy: when the landing is a platform and
/// start_dy is 0 or less, the cast starts just under the platform, so the
/// player stands on the floor below it; a positive start_dy hints at a
/// surface above (for example the same platform). On a miss `out` is left
/// unchanged and the status says what is wrong (including drop heights that
/// cannot be used).
LayoutStatus training_layout_resolve(const Drill* drill, float start_gap,
                                     Layout* out);

const char* training_layout_status_name(LayoutStatus status);

#endif

#endif
