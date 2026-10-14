#ifdef TRAINING_BUILD

#include "main_menu.h"

#include <melee/gm/gm_1601.h>
#include <melee/mn/inlines.h>
#include <melee/mn/mnevent.h>
#include <melee/mn/mnmain.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/mobj.h>
#include <sysdolphin/baselib/tobj.h>

#include "drill.h"

#ifdef AUTHORING_BUILD
#include <dolphin/os.h>
#endif

bool training_main_menu_selectable(int selection)
{
    return selection != SEL_MAIN_TOY;
}

#ifdef AUTHORING_BUILD
/// The rep log's record of a main menu choice.
static void log_choice(int selection)
{
    const char* choice;

    switch (selection) {
    case SEL_MAIN_1P:
        choice = "rtc_101";
        break;
    case SEL_MAIN_VS:
        choice = "custom_drill";
        break;
    case SEL_MAIN_SETTINGS:
        choice = "options";
        break;
    case SEL_MAIN_DATA:
        choice = "data";
        break;
    default:
        return;
    }
    OSReport("[rep] event=menu menu=main choice=%s\n", choice);
}
#define LOG_CHOICE(selection) log_choice(selection)
#else
#define LOG_CHOICE(selection) ((void) 0)
#endif

/* The heading strip's top-level joint is the 84th (in depth-first order) of
 * MenMainPanel_Top. It must stay in step with the joint numbers in
 * tools/archive_patches.py `screen_headings` (84 top level, 85 second level),
 * which is where the archive's textures are replaced. */
#define HEADING_JOINT 84

#ifdef AUTHORING_BUILD
#define HEADING_WARN(why) OSReport("main_menu_heading: %s\n", why)
#else
#define HEADING_WARN(why) ((void) 0)
#endif

static HSD_MatAnimJoint* nth_matanim_joint(HSD_MatAnimJoint* joint, int* count)
{
    while (joint != NULL) {
        HSD_MatAnimJoint* found;

        if ((*count)++ == HEADING_JOINT) {
            return joint;
        }
        found = nth_matanim_joint(joint->child, count);
        if (found != NULL) {
            return found;
        }
        joint = joint->next;
    }
    return NULL;
}

/* Shows the Custom Drill heading (the joint's second image) on the event
 * scene, or puts RTC 101's (its first) back.
 *
 * It depends on the patched archive: tools/archive_patches.py makes the
 * table's first two images RTC 101 and Custom Drill, so a freshly loaded
 * archive always has them different. Showing the Custom Drill heading makes
 * the first entry the second one, which loses the first; so the first is
 * remembered whenever the two differ (a fresh archive, whatever happened
 * before) and put back from there. */
void training_main_menu_heading(bool custom_drill)
{
    static HSD_ImageDesc* rtc_101;
    HSD_MatAnimJoint* joint;
    HSD_ImageDesc** images;
    int count = 0;

    joint = nth_matanim_joint(MenMainPanel_Top.matanim_joint, &count);
    if (joint == NULL || joint->matanim == NULL ||
        joint->matanim->texanim == NULL ||
        joint->matanim->texanim->n_imagetbl < 2)
    {
        HEADING_WARN("no heading joint");
        return;
    }
    images = joint->matanim->texanim->imagetbl;
    if (images[0] != images[1]) {
        rtc_101 = images[0];
    }
    if (custom_drill) {
        images[0] = images[1];
    } else if (images[0] == images[1]) {
        if (rtc_101 == NULL) {
            HEADING_WARN("rtc 101 heading lost");
            return;
        }
        images[0] = rtc_101;
    }
}

/// Opens the event list on `event` as 1-P Mode's Event Match does, and
/// frees the main menu's input gobj.
static void open_event_list(HSD_GObj* gp, int event)
{
    training_main_menu_heading(training_drill_is_custom(event));
    mn_804D6BC8.cooldown = 5;
    mn_804A04F0.entering_menu = 1;
    sfxForward();
    gm_801677E8(mn_802295AC());
    mnEvent_8024E838(event, true);
    HSD_GObjFree(gp);
}

bool training_main_menu_confirm(HSD_GObj* gp)
{
    int selection = mn_804A04F0.hovered_selection;

    LOG_CHOICE(selection);
    switch (selection) {
    case SEL_MAIN_1P:
        open_event_list(gp, 0);
        return true;
    case SEL_MAIN_VS:
        /* The event scene opened on the Custom drill's index, which is past
         * the list's last row: custom_settings.c shows its settings menu on
         * the bare scene. */
        open_event_list(gp, training_drill_custom_index());
        return true;
    default:
        return false;
    }
}

#endif
