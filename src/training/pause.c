#ifdef TRAINING_BUILD

#include "pause.h"

#include "drill.h"
#include "event.h"
#include "layout_editor.h"
#include "locked_camera.h"
#include "menu.h"
#include "punish_window.h"
#include "rep_result.h"
#include "slots.h"
#include "tech_chase.h"

#include <melee/ft/inlines.h>
#include <melee/ft/types.h>
#include <melee/gm/gm_1A36.h>
#include <melee/gm/gmscene.h>
#include <melee/if/ifall.h>
#include <melee/lb/lbaudio_ax.h>
#include <melee/pl/player.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/rumble.h>

#include <dolphin/os.h>
#include <dolphin/pad.h>

#include <stdio.h>

/* The game's pause flag bit for a VS match (what retail VS pause sets). */
#define DB_PAUSE_FLAG_VS 1
/* lbAudioAx_80024030: the retail pause sound. */
#define SOUND_PAUSE 5

/* Task-system placement of the pause GObj and its proc (as the rep loop's). */
#define PAUSE_GOBJ_CLASS 0xE
#define PAUSE_GOBJ_PLINK 2
#define PAUSE_GOBJ_PRIORITY 0
#define PAUSE_PROC_PRIORITY 0x15

static bool paused;
static int pause_port;
/* What the menu's A chose this frame; acted on in think. */
typedef enum Choice {
    Choice_None,
    Choice_Resume,
    Choice_Restart,
    Choice_ChangeCharacters,
    Choice_EventList,
    Choice_EditLayout,
} Choice;

static Choice chosen;
/* Resume was chosen (or the layout editor left): the menu is gone, and the
 * match unfreezes once the A or B that chose it is released (held into the
 * match it would be an attack). */
static bool resume_pending;
/* The layout editor is open over the frozen match, driven by pause_port. */
static bool editor_open;

static void choose_resume(void)
{
    chosen = Choice_Resume;
}

static void choose_restart(void)
{
    chosen = Choice_Restart;
}

static void choose_change_characters(void)
{
    chosen = Choice_ChangeCharacters;
}

static void choose_event_list(void)
{
    chosen = Choice_EventList;
}

static void choose_edit_layout(void)
{
    chosen = Choice_EditLayout;
}


/* The label of the item that goes back to the event list in a curriculum
 * drill (the Custom drill's reads "Settings", see back_label). */
#define BACK_LABEL "Back to RTC 101"

/* The pause menu's items, in order (items[] below). */
enum {
    ITEM_RESUME,
    ITEM_RESTART,
    ITEM_CHANGE_CHARACTERS,
    ITEM_BACK,
};

static TrainingMenuItem items[] = {
    { "Resume", "Carry on with the drill", true, choose_resume },
    { "Restart drill", "Start again from rep 1", true, choose_restart },
    { "Change characters", "Pick the fighters again", true,
      choose_change_characters },
    /* Its label is set when the menu opens (back_label). */
    { BACK_LABEL, "Leave the drill and pick another", true,
      choose_event_list },
    { "Edit layout", "Change the landing, heights and start", true,
      choose_edit_layout },
#ifdef AUTHORING_BUILD
    /* Authoring only: stays paused, so it can be chosen again. */
    { "Print layout", "Write the layout to the rep log", true,
      training_drill_print_layout },
    { "Print camera", "Write the camera view to the rep log", true,
      training_locked_camera_print },
#endif
};

static TrainingMenu menu = { "Paused", items, ARRAY_SIZE(items) };

/// The back item's label: a curriculum drill goes back to RTC 101, the
/// Custom drill to its settings menu.
static const char* back_label(void)
{
    return training_drill_is_custom(training_drill_current_index())
               ? "Settings"
               : BACK_LABEL;
}

#ifdef AUTHORING_BUILD
/// The pause line's extra fields: the punish window shown for each tech
/// (-1 for "--") and the port that paused.
static void log_pause(void)
{
    char extra[128];
    char* p = extra;
    int i;

    p += sprintf(p, " port=%d punish_window=", pause_port);
    for (i = 0; i < training_punish_window_panel_lines(); i++) {
        RepTech tech = training_punish_window_panel_tech(i);

        p += sprintf(p, i == 0 ? "%s:%d" : ",%s:%d",
                     training_rep_tech_log_name(tech),
                     training_punish_window(
                         training_punish_window_opponent_kind(), tech));
    }
    training_tech_chase_log_progress("pause", extra);
}
#define LOG_PAUSE() log_pause()
#define LOG_RESUME(by) training_tech_chase_log_progress("resume", " by=" by)
#else
#define LOG_PAUSE() ((void) 0)
#define LOG_RESUME(by) ((void) 0)
#endif

/// Freezes the match the way retail VS pause does, minus its pause camera
/// and prompts (UnclePunch findings, "Pause"), and opens the menu.
static void pause(int port)
{
    paused = true;
    pause_port = port;
    lbAudioAx_80024E84(1);
    lbAudioAx_80024030(SOUND_PAUSE);
    ifAll_802F3394();
    gm_SetDbPauseFlag(DB_PAUSE_FLAG_VS);
    HSD_PadRumblePauseAll();
    items[ITEM_BACK].label = back_label();
    training_menu_show(&menu);
    training_punish_window_panel_show();
    LOG_PAUSE();
}

static void hide_menu(void)
{
    training_menu_hide(&menu);
    training_punish_window_panel_hide();
}

static void unpause(void)
{
    paused = false;
    hide_menu();
    lbAudioAx_80024E84(0);
    gm_ClearDbPauseFlag(DB_PAUSE_FLAG_VS);
    ifAll_802F33CC();
    HSD_PadRumbleUnpauseAll();
}

/// The port of the first human player whose Start was pressed this frame, or
/// -1.
static int start_port(void)
{
    int slot;

    for (slot = 0; slot < SLOT_COUNT; slot++) {
        if (Player_GetPlayerSlotType(slot) == Gm_PKind_Human) {
            int port = Player_GetPadPort(slot);

            if (gm_GetButtonsTriggered((u8) port) & PAD_BUTTON_START) {
                return port;
            }
        }
    }
    return -1;
}

/// The match can be paused once the player can move (after the Ready/GO
/// countdown).
static bool can_pause(void)
{
    HSD_GObj* player = Player_GetEntity(PLAYER_SLOT);

    return player != NULL && !GET_FIGHTER(player)->input_disabled;
}

/// True when L, R and A are held on a human port and its Start was just
/// pressed: retail's quit chord, which goes to the character select.
static bool quit_chord(void)
{
    int slot;

    for (slot = 0; slot < SLOT_COUNT; slot++) {
        if (Player_GetPlayerSlotType(slot) == Gm_PKind_Human) {
            u8 port = (u8) Player_GetPadPort(slot);
            u64 held = gm_GetButtonsPressed(port);
            u64 chord = PAD_TRIGGER_L | PAD_TRIGGER_R | PAD_BUTTON_A;

            if ((gm_GetButtonsTriggered(port) & PAD_BUTTON_START) &&
                (held & chord) == chord)
            {
                return true;
            }
        }
    }
    return false;
}

/// Ends the match: unfreezes it first (a frozen match cannot end).
static void leave_match(TrainingEventExit exit)
{
    if (paused) {
        unpause();
    }
    training_event_end_match(exit);
}

static void think(HSD_GObj* gobj)
{
    if (quit_chord() && !resume_pending) {
        LOG_RESUME("quit_chord");
        leave_match(TrainingEventExit_CharacterSelect);
        return;
    }
    if (resume_pending) {
        if (!(gm_GetButtonsPressed((u8) pause_port) &
              (PAD_BUTTON_A | PAD_BUTTON_B)))
        {
            resume_pending = false;
            LOG_RESUME("resume");
            unpause();
        }
        return;
    }
    if (editor_open) {
        if (!training_layout_editor_update(pause_port)) {
            editor_open = false;
            resume_pending = true;
        }
        return;
    }
    if (!paused) {
        int port = start_port();

        if (port >= 0 && can_pause()) {
            pause(port);
        }
        return;
    }
    chosen = Choice_None;
    switch (training_menu_update(&menu, pause_port)) {
    case TrainingMenuAction_Close:
        LOG_RESUME("start");
        unpause();
        return;
    case TrainingMenuAction_Back:
        /* B at the top of the pause menu does nothing: only Start or Resume
         * unpauses. */
        break;
    default:
        break;
    }
    switch (chosen) {
    case Choice_Resume:
        hide_menu();
        resume_pending = true;
        break;
    case Choice_Restart:
        training_tech_chase_restart();
        hide_menu();
        resume_pending = true;
        break;
    case Choice_ChangeCharacters:
        leave_match(TrainingEventExit_CharacterSelect);
        break;
    case Choice_EventList:
        leave_match(TrainingEventExit_EventList);
        break;
    case Choice_EditLayout:
        hide_menu();
        training_layout_editor_open();
        editor_open = true;
        break;
    default:
        break;
    }
}

void training_pause_init(void)
{
    paused = false;
    resume_pending = false;
    editor_open = false;
    chosen = Choice_None;
    training_menu_reset(&menu);
    training_layout_editor_init();
    HSD_GObj_SetupProc(
        GObj_Create(PAUSE_GOBJ_CLASS, PAUSE_GOBJ_PLINK, PAUSE_GOBJ_PRIORITY),
        (HSD_GObjEvent) think, PAUSE_PROC_PRIORITY);
}

#endif
