#ifdef TRAINING_BUILD

#include <melee/ft/fighter.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/inlines.h>
#include <melee/ft/kinds/ftCommon/ftCo_DamageFall.h>
#include <melee/ft/types.h>
#include <melee/gm/forward.h>
#include <melee/gm/gm_1A3F.h>
#include <melee/gm/gmscene.h>
#include <melee/gr/stage.h>
#include <melee/pl/player.h>
#include <sysdolphin/baselib/controller.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjproc.h>

/* Slot of the human player and of the CPU opponent in Training Mode. */
#define PLAYER_SLOT 0
#define OPPONENT_SLOT 1

/* Hardcoded layout, relative to the centre of the stage floor. */
#define PLAYER_START_DX -30.0F
#define OPPONENT_DROP_HEIGHT 40.0F

/* Frames to let the scene settle (fighters spawned) before the first rep. */
#define SETTLE_FRAMES 60

typedef enum RepState {
    RepState_Settle,
    RepState_Reset,
    RepState_Drop,
    RepState_Wait,
    RepState_Pause,
} RepState;

/* Frames between the opponent becoming actionable and the next drop. */
#define PAUSE_FRAMES 60

/* A rep that has not ended this many frames after its drop is abandoned, so a
 * rep can never hang (opponent hit away, grabbed, stuck). */
#define REP_TIMEOUT 300

/* Longest wait for a KO'd fighter to respawn before resetting anyway. */
#define RESPAWN_TIMEOUT 600

/* Motion ids (ftCo_MS_*) the lifecycle cares about. */
#define MS_LAST_DEAD_OR_REBIRTH 0x0D
/* Missed-tech knockdown (DownBoundU) through the tech states (PassiveCeil). */
#define MS_KNOCKDOWN_FIRST 0xB7
#define MS_TECH_LAST 0xCC

/* Longest landing prediction, in frames. */
#define PREDICT_MAX 90

/* Task-system placement of the rep-lifecycle GObj and its proc. */
#define TECH_CHASE_GOBJ_CLASS 0xE
#define TECH_CHASE_GOBJ_PLINK 2
#define TECH_CHASE_GOBJ_PRIORITY 0
#define TECH_CHASE_PROC_PRIORITY 0x15

/* gm_GetDbPauseFlag bit that is set while Training Mode is paused. */
#define DB_PAUSE_FLAG_TRAINING 2

/* The cpu.kind the game switches the opponent to on Training pause/unpause (a
 * human-driven type); we put it back to the plain CPU type. */
#define CPU_KIND_HUMAN_DRIVEN 5
#define CPU_TYPE_CPU 0

/* Stage_80224E64 query: centre of the stage floor. */
#define STAGE_QUERY_FLOOR_CENTRE 4

/* Per-rep state, reset together at the start of every rep. */
typedef struct RepData {
    float floor_y;
    bool tech_pressed;
    bool press_pending;
    int rep_frames;
    bool seen_tech;
} RepData;

static RepState rep_state;
static int frames_in_state;
static RepData rep;

static void set_state(RepState s)
{
    rep_state = s;
    frames_in_state = 0;
}

static void reset_rep_data(float floor_y)
{
    rep.floor_y = floor_y;
    rep.tech_pressed = false;
    rep.press_pending = false;
    rep.rep_frames = 0;
    rep.seen_tech = false;
}

static void clear_fighter_motion(Fighter* fp)
{
    fp->self_vel.x = fp->self_vel.y = fp->self_vel.z = 0.0F;
    fp->x8c_kb_vel.x = fp->x8c_kb_vel.y = fp->x8c_kb_vel.z = 0.0F;
    fp->x74_self_accel.x = fp->x74_self_accel.y = fp->x74_self_accel.z = 0.0F;
    fp->gr_vel = 0.0F;
    fp->xF0_ground_kb_vel = 0.0F;
    fp->dmg.x18A4_knockbackMagnitude = 0.0F;
}

/// Reset state: the player stands at the start position facing the opponent
/// at 0%, and the opponent is respawned over the drop point at 0%, facing the
/// player. Then the drop: the opponent enters tumble from rest.
static void do_rep_setup(void)
{
    Vec3 centre;
    Vec3 start;
    Vec3 drop;
    HSD_GObj* opp;

    Stage_80224E64(STAGE_QUERY_FLOOR_CENTRE, &centre);
    reset_rep_data(centre.y);
    start = centre;
    start.x += PLAYER_START_DX;
    drop = centre;
    drop.y += OPPONENT_DROP_HEIGHT;

    Player_SetFacingDirection(PLAYER_SLOT, 1.0F);
    Player_SetHUDDamage(PLAYER_SLOT, 0);
    Player_800328D4(PLAYER_SLOT, &start);

    Player_SetFacingDirection(OPPONENT_SLOT, -1.0F);
    Player_SetHUDDamage(OPPONENT_SLOT, 0);
    Player_800328D4(OPPONENT_SLOT, &drop);

    opp = Player_GetEntity(OPPONENT_SLOT);
    if (opp != NULL) {
        Fighter* fp = GET_FIGHTER(opp);
        clear_fighter_motion(fp);
        ftCo_80090780(opp);
    }
}

static bool in_tech_states(int motion)
{
    return motion >= MS_KNOCKDOWN_FIRST && motion <= MS_TECH_LAST;
}

/// True when the rep is over: the opponent is actionable again (Wait) after
/// its knockdown/tech, or it left the expected path (KO'd), or the rep timed
/// out. Other interruptions (hit away, grabbed) end by timeout.
static bool rep_over(Fighter* fp)
{
    int motion = fp->motion_id;

    rep.rep_frames++;
    if (rep.rep_frames >= REP_TIMEOUT || fp->is_sleeping ||
        motion <= MS_LAST_DEAD_OR_REBIRTH)
    {
        return true;
    }
    if (in_tech_states(motion)) {
        rep.seen_tech = true;
        return false;
    }
    if (rep.seen_tech) {
        return motion == ftCo_MS_Wait;
    }
    return motion != ftCo_MS_DamageFall;
}

/// Frames until the opponent's feet reach the floor, simulating the fall from
/// its current speed with the fighter's own gravity and terminal velocity.
static int predict_landing_frames(Fighter* fp)
{
    float y = fp->cur_pos.y - rep.floor_y;
    float vy = fp->self_vel.y;
    int n = 0;

    while (y > 0.0F && n < PREDICT_MAX) {
        vy -= fp->co_attrs.gravity;
        if (vy < -fp->co_attrs.terminal_velocity) {
            vy = -fp->co_attrs.terminal_velocity;
        }
        y += vy;
        n++;
    }
    return n;
}

/// Decide, from the opponent's fall, whether to press L next frame. The press
/// reaches the game in the next CPU input hook, so aim half the game's tech
/// window before the predicted landing.
static void update_tech_press(Fighter* fp)
{
    int lead = (int) (p_ftCommonData->x250 * 0.5F);

    /* x684 is the frames since the last tech input (the tech lockout); x1C is
     * the game's lockout length. */
    if (!rep.tech_pressed && fp->motion_id == ftCo_MS_DamageFall &&
        fp->self_vel.y < 0.0F && predict_landing_frames(fp) <= lead &&
        fp->x684 >= p_ftCommonData->x1C)
    {
        rep.press_pending = true;
        rep.tech_pressed = true;
    }
}

/// Ends the rep (into Pause) if it is over; true when it did.
static bool end_rep_if_over(Fighter* fp)
{
    if (!rep_over(fp)) {
        return false;
    }
    set_state(RepState_Pause);
    return true;
}

static void think(HSD_GObj* gobj)
{
    HSD_GObj* player = Player_GetEntity(PLAYER_SLOT);
    HSD_GObj* opp = Player_GetEntity(OPPONENT_SLOT);

    if (player == NULL || opp == NULL) {
        return;
    }
    if (gm_GetDbPauseFlag(DB_PAUSE_FLAG_TRAINING)) {
        return;
    }
    /* Training's pause/unpause can switch the CPU to a human-driven type;
     * keep it CPU controlled so our hook silences it. */
    if (GET_FIGHTER(opp)->cpu.kind == CPU_KIND_HUMAN_DRIVEN) {
        Player_SetPlayerAndEntityCpuType(OPPONENT_SLOT, CPU_TYPE_CPU);
    }

    frames_in_state++;
    switch (rep_state) {
    case RepState_Settle:
        if (frames_in_state >= SETTLE_FRAMES) {
            set_state(RepState_Reset);
        }
        break;
    case RepState_Reset:
        /* A KO'd fighter is asleep until the game respawns it, and spawning a
         * sleeping fighter is skipped. Don't wait forever. */
        if ((GET_FIGHTER(opp)->is_sleeping ||
             GET_FIGHTER(player)->is_sleeping) &&
            frames_in_state < RESPAWN_TIMEOUT)
        {
            break;
        }
        do_rep_setup();
        set_state(RepState_Drop);
        break;
    case RepState_Drop:
        update_tech_press(GET_FIGHTER(opp));
        if (!end_rep_if_over(GET_FIGHTER(opp)) && rep.tech_pressed) {
            set_state(RepState_Wait);
        }
        break;
    case RepState_Wait:
        end_rep_if_over(GET_FIGHTER(opp));
        break;
    case RepState_Pause:
        if (frames_in_state >= PAUSE_FRAMES) {
            set_state(RepState_Reset);
        }
        break;
    }
}

/// Called from gm_Scene_Training_OnEnter.
void training_tech_chase_init(void)
{
    set_state(RepState_Settle);
    reset_rep_data(0.0F);
    HSD_GObj_SetupProc(GObj_Create(TECH_CHASE_GOBJ_CLASS,
                                   TECH_CHASE_GOBJ_PLINK,
                                   TECH_CHASE_GOBJ_PRIORITY),
                       (HSD_GObjEvent) think, TECH_CHASE_PROC_PRIORITY);
}

/// Called from Fighter_procCpu after the CPU tick: replaces the opponent's
/// input with neutral so the AI does nothing (ADR-0004). Only in Training
/// Mode: the scene check scopes it without a scene-exit hook.
void training_cpu_input(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);

    if (gm_GetCurrentSceneIndex() != GS_TRAINING ||
        fp->player_idx != OPPONENT_SLOT)
    {
        return;
    }
    fp->cpu.buttons = 0;
    fp->cpu.lstick.x = 0;
    fp->cpu.lstick.y = 0;
    fp->cpu.cstick.x = 0;
    fp->cpu.cstick.y = 0;
    fp->cpu.ltrigger = 0;
    fp->cpu.rtrigger = 0;
    if (rep.press_pending) {
        /* One L press with neutral stick; Fighter_procInput derives the
         * trigger and edge from the button bit. */
        fp->cpu.buttons = HSD_PAD_L;
        rep.press_pending = false;
    }
}

#endif
