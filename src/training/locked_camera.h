#ifndef TRAINING_LOCKED_CAMERA_H
#define TRAINING_LOCKED_CAMERA_H

#ifdef TRAINING_BUILD

#include <stdbool.h>

#include <sysdolphin/baselib/mtx.h>

/* The locked camera: while a drill with vanish on is in play, the camera
 * stays on one view per stage, so its movement can't stand in for the early
 * read of the opponent's tech. */

typedef struct LockedCameraView {
    int stage;     /* external stage id, as in Drill::stage; 0 ends the table */
    Vec3 eye;      /* camera position */
    Vec3 interest; /* the point it looks at */
    float fov;     /* vertical field of view, degrees */
} LockedCameraView;

/// The view for external stage `stage`, or NULL if the stage has none.
const LockedCameraView* training_locked_camera_find(int stage);

/// The view the camera is locked to right now: non-NULL only in a drill event
/// whose current drill has vanish on, on a stage that has a view.
const LockedCameraView* training_locked_camera_view(void);

/// From the camera's per-frame update, after retail's: if the camera is
/// locked, puts it on its view (and keeps it there, quakes included).
void training_locked_camera_update(void);

#ifdef AUTHORING_BUILD
/// Writes the camera as it is now to the rep log as C initializer text for
/// the view table, `[rep] camera: <stage>, { eye }, { interest }, <fov>,`,
/// in hundredths. Only meaningful with the camera unlocked.
void training_locked_camera_print(void);
#endif

#endif

#endif
