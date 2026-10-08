#ifdef TRAINING_BUILD

#include <melee/ft/fighter.h>
#include <melee/ft/ft_0D4D.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/inlines.h>
#include <melee/ft/kinds/ftCommon/ftCo_DamageFall.h>
#include <melee/ft/types.h>
#include <melee/gm/forward.h>
#include <melee/gm/gm_1A3F.h>
#include <melee/gm/gmscene.h>
#include <melee/gr/stage.h>
#include <melee/mp/mpcoll.h>
#include <melee/pl/player.h>
#include <sysdolphin/baselib/controller.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/sislib.h>

#ifdef AUTHORING_BUILD
#include <dolphin/os.h>
#endif

#include "score.h"

/* Slot of the human player and of the CPU opponent in Training Mode. */
#define PLAYER_SLOT 0
#define OPPONENT_SLOT 1

/* Hardcoded layout, relative to the centre of the stage floor. */
#define OPPONENT_DROP_HEIGHT 40.0F

/* How far above and below a spawn point teleport() looks for the floor. */
#define GROUND_PROBE_RANGE 40.0F

/* Frames to let the scene settle (fighters spawned) before the first rep. */
#define SETTLE_FRAMES 60

typedef enum RepState {
    RepState_Settle,
    RepState_Reset,
    RepState_Drop,
    RepState_Wait,
    RepState_Landed,
    RepState_Outcome,
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

/* Rep log (authoring builds only): one `[rep] event=... rep=N frame=F` line per
 * event through OSReport. `frame` counts the frames the rep loop has run since
 * Training Mode was entered (paused frames are not counted). In player builds
 * the REP_LOG* hooks are empty, so there is no logging code or text. */
#ifdef AUTHORING_BUILD
static int rep_number;
static int rep_frame;

static const char* tech_option_name(int motion)
{
    switch (motion) {
    case ftCo_MS_Passive:
        return "in-place";
    case ftCo_MS_PassiveStandF:
        return "forward";
    case ftCo_MS_PassiveStandB:
        return "back";
    case ftCo_MS_PassiveWall:
        return "wall";
    case ftCo_MS_PassiveWallJump:
        return "walljump";
    case ftCo_MS_PassiveCeil:
        return "ceil";
    default:
        return "miss";
    }
}

/* The common `[rep] event=E rep=N frame=F` prefix; `extra` is a printf format
 * for trailing fields (its arguments follow in REP_LOG_WITH). */
#define REP_LOG_FMT(event, extra)                                             \
    "[rep] event=" event " rep=%d frame=%d" extra "\n"
#define REP_LOG(event)                                                        \
    OSReport(REP_LOG_FMT(event, ""), rep_number, rep_frame)
#define REP_LOG_WITH(event, extra, ...)                                       \
    OSReport(REP_LOG_FMT(event, extra), rep_number, rep_frame, __VA_ARGS__)
#define REP_LOG_RESET() (rep_number = rep_frame = 0)
#define REP_LOG_TICK() (rep_frame++)
#define REP_LOG_START() (rep_number++, REP_LOG("start"))
#else
#define REP_LOG(event) ((void) 0)
#define REP_LOG_WITH(...) ((void) 0)
#define REP_LOG_RESET() ((void) 0)
#define REP_LOG_TICK() ((void) 0)
#define REP_LOG_START() ((void) 0)
#endif

/* How the player's punish reached the opponent. */
typedef enum HitKind {
    HitKind_None,
    HitKind_Hit,
    HitKind_Grab,
} HitKind;

#ifdef AUTHORING_BUILD
static const char* hit_kind_name(HitKind kind)
{
    switch (kind) {
    case HitKind_Grab:
        return "grab";
    case HitKind_Hit:
        return "hit";
    default:
        return "none";
    }
}
#endif

/* Frame number meaning "has not happened (yet) in this rep". */
#define NO_FRAME (-1)

/* Per-rep state, reset together at the start of every rep. Frames are counted
 * from the rep's drop (rep_frames), so they match across reps. */
typedef struct RepData {
    float floor_y;
    bool tech_pressed;
    bool press_pending;
    int rep_frames;
    bool seen_tech;
    /* The punish window, measured from the opponent's live state: it opens on
     * the landing frame and closes on the first actionable frame. The
     * vulnerable frames are the window frames in which the opponent can be
     * hit or grabbed. */
    int window_open;
    int vuln_from;
    int vuln_to;
    int actionable;
    /* The player's hit on the opponent: the frame it happened, or NO_FRAME. */
    int hit_frame;
    HitKind hit_kind;
    /* The rep was voided by an early hit. */
    bool voided;
} RepData;

/* The opponent's damage percent as of the previous frame, to see when it takes
 * damage. Updated every frame, in every state. */
static float prev_percent;

/* Whether the opponent was held by the player's fighter as of the previous
 * frame, to see the frame the grab connects. Updated every frame. */
static bool prev_grabbed;

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
    rep.window_open = NO_FRAME;
    rep.vuln_from = NO_FRAME;
    rep.vuln_to = NO_FRAME;
    rep.actionable = NO_FRAME;
    rep.hit_frame = NO_FRAME;
    rep.hit_kind = HitKind_None;
    rep.voided = false;
}

/// Records the player's hit or grab on the opponent at the current rep frame.
static void record_hit(HitKind hit)
{
    rep.hit_frame = rep.rep_frames;
    rep.hit_kind = hit;
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

/// True when the opponent is in a captured state (pulled, held or damaged
/// while held) and the fighter holding it is the player's. The game does not
/// credit grabs to the attacker slot, so the grabber link on the captured
/// fighter is what attributes it.
static bool opponent_grabbed_by_player(Fighter* fp)
{
    return fp->motion_id >= ftCo_MS_CapturePulledHi &&
           fp->motion_id <= ftCo_MS_CaptureDamageLw &&
           fp->victim_gobj != NULL &&
           fp->victim_gobj == Player_GetEntity(PLAYER_SLOT);
}

/// Re-syncs the hit baselines with the opponent as it is now, so a reset
/// (percent back to 0, a grab dropped) cannot read as a hit or hide one on the
/// next frame. Polling only runs while the rep loop does.
static void refresh_hit_baseline(Fighter* fp)
{
    prev_percent = fp->dmg.x1830_percent;
    prev_grabbed = opponent_grabbed_by_player(fp);
}

/// Moves a fighter to `pos` and respawns it there, like Player_800328D4 but
/// without the spawn sparkles (a distraction in a drill that needs focus):
/// ftCo_800D4F24's non-zero index is what spawns them.
///
/// With `grounded`, the fighter ends the teleport standing on the floor below
/// `pos` (in Wait), so its first frame of input reads as a grounded one.
/// Fighter_Spawn leaves it airborne (its own ground probe, ft_80082A68, only
/// reaches 10 units and misses); the probe here reaches GROUND_PROBE_RANGE.
/// The probe also loads the fighter's real ECB, which a spawn leaves stale.
static void teleport(int slot, Vec3* pos, bool grounded)
{
    HSD_GObj* gobj;
    Fighter* fp;
    CollData* coll;
    bool found;

    Player_80032768(slot, pos);
    gobj = Player_GetEntity(slot);
    if (gobj == NULL || GET_FIGHTER(gobj)->is_sleeping) {
        return;
    }
    fp = GET_FIGHTER(gobj);
    coll = &fp->coll_data;
    ftCo_800D4F24(gobj, 0);
    ftCommon_8007ED2C(fp);
    Fighter_Spawn(gobj);

    coll->cur_pos = fp->cur_pos;
    coll->last_pos = fp->cur_pos;
    coll->last_pos.y += GROUND_PROBE_RANGE;
    coll->cur_pos.y -= GROUND_PROBE_RANGE;
    found = mpColl_800471F8(coll);
    if (grounded && found) {
        fp->cur_pos = coll->cur_pos;
        ftCommon_8007D6A4(fp);
    } else {
        coll->cur_pos = fp->cur_pos;
        coll->last_pos = fp->cur_pos;
    }
    ftCommon_8007D92C(gobj);
}

/* How far each fighter's body reaches to the side that faces the other, from
 * its own position, measured while both stand in Wait (see measure_reach). The
 * player starts exactly their sum from the opponent: as close as possible
 * without standing inside the opponent. */
static float player_reach;
static float opponent_reach;

/// The farthest an enabled hurtbox of `fp` reaches along the x axis towards
/// `side` (+1 right, -1 left), measured from its position. Hurtbox positions
/// follow the live pose, so this is only meaningful when the pose is fresh.
static float hurtbox_reach(Fighter* fp, float side)
{
    float reach = 0.0F;
    int i;

    for (i = 0; i < fp->hurt_capsules_len; i++) {
        HurtCapsule* hc = &fp->hurt_capsules[i].capsule;
        float a;
        float b;
        float far;

        if (hc->state != HurtCapsule_Enabled) {
            continue;
        }
        a = (hc->a_pos.x - fp->cur_pos.x) * side;
        b = (hc->b_pos.x - fp->cur_pos.x) * side;
        far = (a > b ? a : b) + hc->scale;
        if (far > reach) {
            reach = far;
        }
    }
    return reach;
}

/// Measures both fighters' reach towards each other. Called at the end of
/// Settle, when both stand in Wait with fresh hurtboxes; the same every rep
/// after, so the start spacing never depends on the previous rep's pose.
static void measure_reach(HSD_GObj* player, HSD_GObj* opp)
{
    player_reach = hurtbox_reach(GET_FIGHTER(player), 1.0F);
    opponent_reach = hurtbox_reach(GET_FIGHTER(opp), -1.0F);
}

/// Reset state: the player stands next to the opponent's drop point facing it
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
    start.x -= player_reach + opponent_reach;
    drop = centre;
    drop.y += OPPONENT_DROP_HEIGHT;

    Player_SetFacingDirection(PLAYER_SLOT, 1.0F);
    Player_SetHUDDamage(PLAYER_SLOT, 0);
    teleport(PLAYER_SLOT, &start, true);

    Player_SetFacingDirection(OPPONENT_SLOT, -1.0F);
    Player_SetHUDDamage(OPPONENT_SLOT, 0);
    teleport(OPPONENT_SLOT, &drop, false);
    REP_LOG_START();
    REP_LOG_WITH("drop", " height=%d", (int) OPPONENT_DROP_HEIGHT);

    opp = Player_GetEntity(OPPONENT_SLOT);
    if (opp != NULL) {
        Fighter* fp = GET_FIGHTER(opp);
        clear_fighter_motion(fp);
        ftCo_80090780(opp);
        refresh_hit_baseline(fp);
    }
}

static bool in_tech_states(int motion)
{
    return motion >= MS_KNOCKDOWN_FIRST && motion <= MS_TECH_LAST;
}

/// True when the opponent can be hit or grabbed this frame: no whole-body
/// invincibility or intangibility, and at least one hurtbox enabled. The game
/// sets all of these per character and per state, so nothing is tabulated.
static bool opponent_vulnerable(Fighter* fp)
{
    int i;

    if (fp->x1988 != 0 || fp->x198C != 0 || fp->x221D_b6) {
        return false;
    }
    for (i = 0; i < fp->hurt_capsules_len; i++) {
        if (fp->hurt_capsules[i].capsule.state == HurtCapsule_Enabled) {
            return true;
        }
    }
    return false;
}

/// True when the rep ended before the opponent landed: it timed out, or the
/// opponent was KO'd. Expects the frame to be counted already.
static bool rep_aborted(Fighter* fp)
{
    return rep.rep_frames >= REP_TIMEOUT || fp->is_sleeping ||
           fp->motion_id <= MS_LAST_DEAD_OR_REBIRTH;
}

/// How the player reached the opponent this frame, or HitKind_None. A hit is
/// the opponent taking damage with the recorded attacker the player's slot; a
/// grab is the opponent newly captured by the player's fighter. Polled every
/// frame, so the baselines always follow the opponent; damage or grabs from
/// other sources are not credited.
static HitKind poll_player_hit(Fighter* fp)
{
    bool took_damage = fp->dmg.x1830_percent > prev_percent;
    bool grabbed = opponent_grabbed_by_player(fp);
    bool new_grab = grabbed && !prev_grabbed;

    prev_percent = fp->dmg.x1830_percent;
    prev_grabbed = grabbed;
    if (new_grab) {
        return HitKind_Grab;
    }
    if (took_damage && fp->dmg.x18c4_source_ply == PLAYER_SLOT) {
        return HitKind_Hit;
    }
    return HitKind_None;
}

/// Opens the punish window on the landing frame.
static void open_window(Fighter* fp)
{
    rep.seen_tech = true;
    rep.window_open = rep.rep_frames;
    REP_LOG("landing");
    REP_LOG_WITH("tech", " option=%s", tech_option_name(fp->motion_id));
}

/// Measures the current frame of the punish window. True when the window
/// closed this frame (the opponent is actionable, back in Wait) or the rep
/// timed out. Expects the frame to be counted already.
static bool measure_window(Fighter* fp)
{
    if (opponent_vulnerable(fp)) {
        if (rep.vuln_from == NO_FRAME) {
            rep.vuln_from = rep.rep_frames;
        }
        rep.vuln_to = rep.rep_frames;
    }
    if (fp->motion_id == ftCo_MS_Wait) {
        rep.actionable = rep.rep_frames;
        REP_LOG("actionable");
        return true;
    }
    return rep.rep_frames >= REP_TIMEOUT;
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

/// Writes the rep's outcome line. The window fields are -1 when the window
/// never opened or closed; hit_frame is -1 and hit_kind=none for no punish.
static void log_outcome(const char* outcome)
{
    REP_LOG_WITH("outcome",
                 " outcome=%s window_open=%d vuln_from=%d vuln_to=%d "
                 "actionable=%d hit_frame=%d hit_kind=%s",
                 outcome, rep.window_open, rep.vuln_from, rep.vuln_to,
                 rep.actionable, rep.hit_frame,
                 hit_kind_name(rep.hit_kind));
}

/* The "early" notice shown for a void rep: position (canvas units) and size.
 * Tune by eye with `run`. The text is the full-width Shift-JIS the game's SIS
 * font expects. */
#define EARLY_X 270.0F
#define EARLY_Y 150.0F
#define EARLY_SCALE 1.4F
static char early_str[] = "\x82\x85\x82\x81\x82\x92\x82\x8C\x82\x99";

static HSD_Text* early_text;

/// Creates the "early" text, hidden, once the scene has settled; text made
/// during the scene's own setup is not drawn. Showing and hiding it flips its
/// hidden flag.
static void create_early(void)
{
    int idx;

    early_text = training_text_create();
    idx = HSD_SisLib_803A6B98(early_text, EARLY_X, EARLY_Y, "%s", early_str);
    HSD_SisLib_803A7548(early_text, idx, EARLY_SCALE, EARLY_SCALE);
    early_text->hidden = true;
}

static void show_early(void)
{
    if (early_text != NULL) {
        early_text->hidden = false;
    }
}

static void hide_early(void)
{
    if (early_text != NULL) {
        early_text->hidden = true;
    }
}

/// Per-frame check for the Drop and Wait states: ends the rep (into Pause) if
/// it was aborted, or if the player hit the opponent before it landed (void,
/// "early" shown), or moves to Landed, opening the punish window, when the
/// opponent lands (straight to Outcome, as a success, if it is hit that same
/// frame). True when the state changed.
static bool advance_if_landed_or_aborted(Fighter* fp, HitKind hit)
{
    /* The frame is counted here for Drop and Wait; the Landed state counts its
     * own. rep_aborted and measure_window rely on the count being current. */
    rep.rep_frames++;
    if (rep_aborted(fp)) {
        set_state(RepState_Pause);
        return true;
    }
    if (in_tech_states(fp->motion_id)) {
        open_window(fp);
        measure_window(fp);
        if (hit != HitKind_None) {
            record_hit(hit);
        }
        set_state(hit != HitKind_None ? RepState_Outcome : RepState_Landed);
        return true;
    }
    if (hit != HitKind_None) {
        record_hit(hit);
        log_outcome("void");
        rep.voided = true;
        set_state(RepState_Pause);
        return true;
    }
    return false;
}

/// The outcome step: the rep ends as a success (the player hit the opponent
/// inside the punish window) or a failure (the opponent became actionable
/// unpunished) and is scored. A window that timed out logs nothing.
static void finish_rep(void)
{
    if (rep.hit_frame != NO_FRAME) {
        training_score_record(true);
        log_outcome("success");
    } else if (rep.actionable != NO_FRAME) {
        training_score_record(false);
        log_outcome("failure");
    }
}

static void think(HSD_GObj* gobj)
{
    HSD_GObj* player = Player_GetEntity(PLAYER_SLOT);
    HSD_GObj* opp = Player_GetEntity(OPPONENT_SLOT);
    HitKind hit;

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
    REP_LOG_TICK();
    hit = poll_player_hit(GET_FIGHTER(opp));
    switch (rep_state) {
    case RepState_Settle:
        if (frames_in_state == 1) {
            training_score_show();
            create_early();
        }
        if (frames_in_state >= SETTLE_FRAMES) {
            measure_reach(player, opp);
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
        hide_early();
        do_rep_setup();
        set_state(RepState_Drop);
        break;
    case RepState_Drop:
        update_tech_press(GET_FIGHTER(opp));
        if (!advance_if_landed_or_aborted(GET_FIGHTER(opp), hit) &&
            rep.tech_pressed)
        {
            set_state(RepState_Wait);
        }
        break;
    case RepState_Wait:
        advance_if_landed_or_aborted(GET_FIGHTER(opp), hit);
        break;
    case RepState_Landed:
        /* Count this frame before measuring it (see
         * advance_if_landed_or_aborted for Drop and Wait). */
        rep.rep_frames++;
        if (measure_window(GET_FIGHTER(opp))) {
            set_state(RepState_Outcome);
        } else if (hit != HitKind_None) {
            /* Hit or grab inside the window: success, the rep ends at once. */
            record_hit(hit);
            set_state(RepState_Outcome);
        }
        break;
    case RepState_Outcome:
        finish_rep();
        set_state(RepState_Pause);
        break;
    case RepState_Pause:
        if (rep.voided) {
            show_early();
        }
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
    early_text = NULL;
    prev_percent = 0.0F;
    prev_grabbed = false;
    REP_LOG_RESET();
    reset_rep_data(0.0F);
    training_score_init();
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
