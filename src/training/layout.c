#ifdef TRAINING_BUILD

#include "layout.h"

#include <melee/mp/mplib.h>

/* The landing point's cast starts this far above its y hint, so a hint on a
 * surface (or a little below it) still finds that surface. Every cast ends
 * this far below its start: past any stage's lowest floor. */
#define RAY_ABOVE_HINT 10.0F
#define RAY_BELOW_START 1000.0F

/* A cast "from under a surface" starts this far below it, which clears the
 * surface itself so the cast finds the floor beneath. */
#define SURFACE_CLEARANCE 1.0F

/* mpCheckMultiple `checks` bit for floors only. */
#define CHECK_FLOORS 1

/// Casts straight down at `x`, from `from_y`, and puts the first floor it
/// meets in `out`. False if there is none.
static bool find_floor_below(float x, float from_y, Vec3* out)
{
    return mpCheckMultiple(x, from_y, x, from_y - RAY_BELOW_START, out, NULL,
                           NULL, NULL, CHECK_FLOORS, -1, -1);
}

/// True when the drill's heights are usable: a list has 1 to DRILL_MAX_HEIGHTS
/// of them, a range has its two ends.
static bool heights_valid(const Drill* drill)
{
    if (drill->height_kind == DrillHeight_Range) {
        return drill->height_count == 2;
    }
    return drill->height_count >= 1 &&
           drill->height_count <= DRILL_MAX_HEIGHTS;
}

LayoutStatus training_layout_resolve(const Drill* drill, float start_gap,
                                     Layout* out)
{
    Layout layout;
    Vec3 below;
    bool elevated;
    float hint_y;
    float from_y;
    float start_x;

    if (!heights_valid(drill)) {
        return Layout_BadHeights;
    }
    if (!find_floor_below(drill->landing_x, drill->landing_y + RAY_ABOVE_HINT,
                          &layout.landing))
    {
        return Layout_NoLandingFloor;
    }
    /* A landing surface with a floor beneath it is a platform. */
    elevated = find_floor_below(layout.landing.x,
                                layout.landing.y - SURFACE_CLEARANCE, &below);

    start_x = layout.landing.x - start_gap + drill->start_dx;
    hint_y = layout.landing.y + drill->start_dy;
    /* With no offset up, a platform landing puts the player on the floor
     * beneath it (the cast starts under the platform); otherwise the hint
     * chooses the surface, like the landing's. */
    from_y = (elevated && drill->start_dy <= 0.0F)
                 ? hint_y - SURFACE_CLEARANCE
                 : hint_y + RAY_ABOVE_HINT;
    if (!find_floor_below(start_x, from_y, &layout.start)) {
        return Layout_NoStartFloor;
    }
    *out = layout;
    return Layout_Ok;
}

const char* training_layout_status_name(LayoutStatus status)
{
    switch (status) {
    case Layout_NoLandingFloor:
        return "no-floor-below-landing";
    case Layout_NoStartFloor:
        return "no-floor-below-start";
    case Layout_BadHeights:
        return "bad-drop-heights";
    default:
        return "ok";
    }
}

#endif
