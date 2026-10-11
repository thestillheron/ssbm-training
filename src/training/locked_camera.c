#ifdef TRAINING_BUILD

#include "locked_camera.h"

#include "drill.h"
#include "event.h"

#include <melee/cm/camera.h>

#ifdef AUTHORING_BUILD
#include <dolphin/os.h>

#include <stdio.h>
#endif

/* One view per tournament-legal stage, measured with both fighters on the
 * main floor at the two ledges (the authoring Print camera item and the
 * locked-camera-capture-* scenarios; see docs/developer-workflow.md). */
static const LockedCameraView views[] = {
    /* Fountain of Dreams */
    { DRILL_STAGE_FOUNTAIN_OF_DREAMS,
      { 1.13F, 21.39F, 236.70F },
      { 1.92F, 6.15F, 0.00F },
      30.00F },
    /* Pokemon Stadium */
    { DRILL_STAGE_POKEMON_STADIUM,
      { -0.83F, 46.93F, 311.50F },
      { 1.79F, 26.76F, 0.00F },
      30.00F },
    /* Yoshi's Story */
    { DRILL_STAGE_YOSHIS_STORY,
      { 1.45F, 43.24F, 328.81F },
      { 2.24F, 9.13F, 0.00F },
      20.00F },
    /* Dream Land */
    { DRILL_STAGE_DREAM_LAND,
      { -0.54F, 15.45F, 279.37F },
      { 1.81F, 3.59F, 0.00F },
      30.00F },
    /* Battlefield */
    { DRILL_STAGE_BATTLEFIELD,
      { 1.12F, 48.65F, 273.43F },
      { 2.85F, 29.07F, 0.00F },
      30.00F },
    /* Final Destination */
    { DRILL_STAGE_FINAL_DESTINATION,
      { 1.87F, 33.37F, 326.06F },
      { 2.90F, 10.76F, 0.00F },
      30.00F },
    { 0 },
};

const LockedCameraView* training_locked_camera_find(int stage)
{
    const LockedCameraView* view;

    for (view = views; view->stage != 0; view++) {
        if (view->stage == stage) {
            return view;
        }
    }
    return NULL;
}

const LockedCameraView* training_locked_camera_view(void)
{
    const Drill* drill;

    if (!training_event_drill_in_play()) {
        return NULL;
    }
    drill = training_drill_current();
    return drill->vanish ? training_locked_camera_find(drill->stage) : NULL;
}

void training_locked_camera_update(void)
{
    const LockedCameraView* view = training_locked_camera_view();

    if (view != NULL) {
        Camera_TrainingSetView(&view->eye, &view->interest, view->fov);
    }
}

#ifdef AUTHORING_BUILD
/// Appends `value` as a C float literal rounded to hundredths ("-7.25F").
static char* print_hundredths(char* p, float value)
{
    int n = (int) (value * 100.0F + (value < 0 ? -0.5F : 0.5F));

    if (n < 0) {
        *p++ = '-';
        n = -n;
    }
    return p + sprintf(p, "%d.%02dF", n / 100, n % 100);
}

static char* print_vec(char* p, const Vec3* v)
{
    p += sprintf(p, "{ ");
    p = print_hundredths(p, v->x);
    p += sprintf(p, ", ");
    p = print_hundredths(p, v->y);
    p += sprintf(p, ", ");
    p = print_hundredths(p, v->z);
    p += sprintf(p, " }");
    return p;
}

void training_locked_camera_print(void)
{
    char line[160];
    char* p = line;
    Vec3 eye;
    Vec3 interest;
    float fov;

    Camera_TrainingGetView(&eye, &interest, &fov);
    p += sprintf(p, "%d, ", training_drill_current()->stage);
    p = print_vec(p, &eye);
    p += sprintf(p, ", ");
    p = print_vec(p, &interest);
    p += sprintf(p, ", ");
    p = print_hundredths(p, fov);
    OSReport("[rep] camera: %s,\n", line);
}
#endif

#endif
