#ifdef TRAINING_BUILD

#include <melee/ft/fighter.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/ft_0D4D.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/inlines.h>
#include <melee/ft/kinds/ftCommon/ftCo_DamageFall.h>
#include <melee/ft/types.h>
#include <melee/gm/forward.h>
#include <melee/gm/gm_1A3F.h>
#include <melee/gm/gmscene.h>
#include <melee/mp/mpcoll.h>
#include <melee/pl/player.h>
#include <sysdolphin/baselib/controller.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/random.h>
#include <sysdolphin/baselib/sislib.h>

#ifdef AUTHORING_BUILD
#include <dolphin/os.h>
#endif

#include "drill.h"
#include "event.h"
#include "layout.h"
#include "menu.h"
#include "punish_window.h"
#include "rep_result.h"
#include "score.h"
#include "tech_chase.h"
#include "slots.h"
#include "text.h"
#include "tech_options.h"

/* The layout and settings come from the current drill (drill.h); layout.h
 * snaps its points onto the stage. */

/* How far above and below a spawn point teleport() looks for the floor. The
 * layout has snapped the point onto a surface, so this only absorbs rounding. */
#define GROUND_PROBE_RANGE 40.0F

/* Vanish: the opponent is drawn for this many frames of its tech, counted
 * from the landing frame as frame 1, then hidden until its first vulnerable
 * frame (see .scratch/tech-vanish/spec.md). */
#define VANISH_VISIBLE_FRAMES 7

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
    RepState_Summary,
    RepState_Invalid, /* the drill's layout has no floor: nothing runs */
} RepState;

/* Frames between the opponent becoming actionable and the next drop. */
#define PAUSE_FRAMES 120

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

/* The gm_GetDbPauseFlag bit set while a VS match (a drill event) is paused. */
#define DB_PAUSE_FLAG_VS 1

/// Which side of the opponent the player stands on: 1 for the right, -1 for
/// the left. The one rule for "toward the player", shared by the stick that
/// forces a roll and the check of the roll that came out: if the two x
/// positions are equal, the opponent's facing counts as toward.
static int side_of_player(float opp_x, float opp_facing, float player_x)
{
    float dx = player_x - opp_x;

    if (dx == 0.0F) {
        return opp_facing > 0.0F ? 1 : -1;
    }
    return dx > 0.0F ? 1 : -1;
}

/// The player's x position.
static float player_x(void)
{
    return GET_FIGHTER(Player_GetEntity(PLAYER_SLOT))->cur_pos.x;
}

/* What the game actually entered on landing, kept apart from TechOption (what
 * the drill aimed for): a result can be one no option aims for (wall and
 * ceiling techs), and a failed press knocks the opponent down, which is only
 * what the drill wanted when the option was missed tech. */
typedef enum TechResult {
    TechResult_InPlace,
    TechResult_In,
    TechResult_Away,
    TechResult_Knockdown,
    TechResult_Wall,
    TechResult_WallJump,
    TechResult_Ceil,
    TechResult_Count,
} TechResult;

/* What each landing result amounts to, in TechResult's order: the tech the
 * rep result and the summary name, and the rep log's `result_state`. */
static const struct {
    RepTech tech;
    REP_LOG_ONLY(const char* log_name;)
} tech_results[TechResult_Count] = {
    { RepTech_InPlace REP_LOG_ONLY(, "in-place") },
    { RepTech_In REP_LOG_ONLY(, "in") },
    { RepTech_Away REP_LOG_ONLY(, "away") },
    { RepTech_Missed REP_LOG_ONLY(, "knockdown") },
    { RepTech_Wall REP_LOG_ONLY(, "wall") },
    { RepTech_Wall REP_LOG_ONLY(, "walljump") },
    { RepTech_Ceiling REP_LOG_ONLY(, "ceil") },
};

/// Maps the opponent's landing state to what it amounts to. A roll is forward
/// or back relative to the opponent's facing; the player-relative name follows
/// from which side of the opponent the player is on.
static TechResult tech_result(Fighter* fp, float px)
{
    bool player_ahead =
        side_of_player(fp->cur_pos.x, fp->facing_dir, px) * fp->facing_dir >
        0.0F;

    switch (fp->motion_id) {
    case ftCo_MS_Passive:
        return TechResult_InPlace;
    case ftCo_MS_PassiveStandF:
        return player_ahead ? TechResult_In : TechResult_Away;
    case ftCo_MS_PassiveStandB:
        return player_ahead ? TechResult_Away : TechResult_In;
    case ftCo_MS_PassiveWall:
        return TechResult_Wall;
    case ftCo_MS_PassiveWallJump:
        return TechResult_WallJump;
    case ftCo_MS_PassiveCeil:
        return TechResult_Ceil;
    default:
        return TechResult_Knockdown;
    }
}

/* Rep log (authoring builds only): one `[rep] event=... rep=N frame=F` line per
 * event through OSReport. `frame` counts the frames the rep loop has run since
 * the drill event started (paused frames are not counted). In player builds
 * the REP_LOG* hooks are empty, so there is no logging code or text. */
#ifdef AUTHORING_BUILD
static int rep_number;
static int rep_frame;

static const char* tech_option_name(TechOption option)
{
    return training_rep_tech_log_name(training_rep_tech_of_option(option));
}

/// True when the game did what the option aimed for.
static bool result_matches(TechOption option, TechResult result)
{
    return tech_results[result].tech == training_rep_tech_of_option(option);
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
#define REP_LOG_RESTART() (rep_number = 0)

/// Writes the `drill_end` line, then an `option_summary` line for each tech
/// the opponent did in a counted rep: what the summary's option table shows.
static void log_drill_end(const Drill* drill)
{
    int successes = training_score_successes();
    int i;

    REP_LOG_WITH("drill_end", " score=%d/%d pass_mark=%d passed=%s", successes,
                 training_score_counted(), drill->pass_mark,
                 training_score_passed(drill) ? "true" : "false");
    for (i = 0; i < RepTech_Count; i++) {
        TechScore o = training_score_tech((RepTech) i);

        if (o.counted != 0) {
            REP_LOG_WITH("option_summary",
                         " option=%s counted=%d successes=%d early=%d late=%d",
                         training_rep_tech_log_name((RepTech) i), o.counted,
                         o.successes, o.early, o.late);
        }
    }
}
#define LOG_INVALID_LAYOUT(drill, status)                                     \
    OSReport("[rep] event=invalid_layout drill=\"%s\" problem=%s\n",        \
             (drill)->name, training_layout_status_name(status))
#define LOG_LAYOUT(layout)                                                    \
    REP_LOG_WITH("layout",                                                    \
                 " landing_x=%d landing_y=%d start_x=%d start_y=%d",          \
                 (int) (layout)->landing.x, (int) (layout)->landing.y,        \
                 (int) (layout)->start.x, (int) (layout)->start.y)

static void log_drill_start(const Drill* drill)
{
    int i;

    OSReport("[rep] event=drill_start name=\"%s\" stage=%d height_kind=%s "
             "heights=",
             drill->name, drill->stage,
             drill->height_kind == DrillHeight_Range ? "range" : "list");
    for (i = 0; i < drill->height_count && i < DRILL_MAX_HEIGHTS; i++) {
        OSReport(i == 0 ? "%d" : ",%d", (int) drill->heights[i]);
    }
    OSReport(" landing_x=%d landing_y=%d start_dx=%d start_dy=%d",
             (int) drill->landing_x, (int) drill->landing_y,
             (int) drill->start_dx, (int) drill->start_dy);
    OSReport(" pool=%d vanish=%d reset=%d facing=%d length=%s/%d pass=%d\n",
             drill->option_pool, drill->vanish, drill->reset_player,
             drill->opponent_facing,
             drill->length_kind == DrillLength_Time ? "time" : "reps",
             drill->length, drill->pass_mark);
}
#else
#define REP_LOG(event) ((void) 0)
#define REP_LOG_WITH(...) ((void) 0)
#define REP_LOG_RESET() ((void) 0)
#define REP_LOG_TICK() ((void) 0)
#define REP_LOG_START() ((void) 0)
#define REP_LOG_RESTART() ((void) 0)
#define log_drill_end(drill) ((void) 0)
#define LOG_INVALID_LAYOUT(drill, status) ((void) 0)
#define LOG_LAYOUT(layout) ((void) 0)
#define log_drill_start(drill) ((void) 0)
#endif

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

/* Per-rep state, reset together at the start of every rep. Frames are counted
 * from the rep's drop (rep_frames), so they match across reps. */
typedef struct RepData {
    float floor_y;
    bool tech_pressed;
    bool press_pending;
    int rep_frames;
    bool seen_tech;
    /* The punish window and the player's hits, measured from the
     * opponent's live state (rep_result.h). */
    RepFrames frames;
    /* frames.early_attack_frame is not yet followed by a vulnerable or
     * actionable frame, so its frames early are not known yet. */
    bool early_pending;
    /* The rep was scored as a failure, so its timing class goes into the
     * summary's early and late counts once final (at the end of the pause). */
    bool scored_failure;
    /* The tech option chosen for this rep, before the drop. */
    TechOption option;
    /* What the opponent did on landing (set when the window opens). */
    RepTech tech;
    /* Vanish: the opponent is hidden now; the first hidden frame. */
    bool vanish_hidden;
    int vanish_from;
} RepData;

/* The opponent's damage percent as of the previous frame, to see when it takes
 * damage. Updated every frame, in every state. */
static float prev_percent;

/* Whether the opponent was held by the player's fighter as of the previous
 * frame, to see the frame the grab connects. Updated every frame. */
static bool prev_grabbed;

/* Set by every run of the rep loop (with both fighters present) and cleared by
 * the opponent's input hook: the hook acts only on a frame the loop ran, i.e.
 * in a drill event. */
static bool rep_loop_ran;

/* Whether the player has been placed since the drill event started: with reset
 * player off, only the first rep places them. */
static bool player_placed;

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
    rep.frames.window_open = NO_FRAME;
    rep.frames.vuln_from = NO_FRAME;
    rep.frames.vuln_to = NO_FRAME;
    rep.frames.actionable = NO_FRAME;
    rep.frames.predicted_vuln_to = NO_FRAME;
    rep.frames.hit.frame = NO_FRAME;
    rep.frames.hit.kind = HitKind_None;
    rep.frames.early_attack_frame = NO_FRAME;
    rep.frames.early_frames = NO_FRAME;
    rep.early_pending = false;
    rep.frames.late_hit.frame = NO_FRAME;
    rep.frames.late_hit.kind = HitKind_None;
    rep.frames.voided = false;
    rep.scored_failure = false;
    rep.option = TechOption_InPlace;
    rep.tech = RepTech_InPlace;
    rep.vanish_hidden = false;
    rep.vanish_from = NO_FRAME;
}

/// The option chosen for the current rep (see tech_options.h).
TechOption training_tech_option(void)
{
    return rep.option;
}

/// Picks the rep's tech option uniformly from the pool with the game's random
/// number function. The pool must not be empty.
static TechOption choose_tech_option(void)
{
    int count = 0;
    int pick;
    int i;

    for (i = 0; i < TechOption_Count; i++) {
        if (training_drill_current()->option_pool & TECH_OPTION_BIT(i)) {
            count++;
        }
    }
    pick = HSD_Randi(count);
    for (i = 0;; i++) {
        if ((training_drill_current()->option_pool & TECH_OPTION_BIT(i)) && pick-- == 0) {
            return (TechOption) i;
        }
    }
}

/// Records the player's hit or grab on the opponent at the current rep frame.
static void record_hit(HitKind hit)
{
    rep.frames.hit.frame = rep.rep_frames;
    rep.frames.hit.kind = hit;
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

/// The stick x (full left or right) that rolls the opponent toward the player
/// (tech in) or away (tech away), from where the two stand now.
static s8 roll_stick_x(float opp_x, float opp_facing, float px,
                       TechOption option)
{
    bool toward_right = side_of_player(opp_x, opp_facing, px) > 0;

    if (option == TechOption_Away) {
        toward_right = !toward_right;
    }
    return toward_right ? 127 : -127;
}

/// The opponent's facing (+1 right, -1 left) for a drill's setting, when it is
/// respawned over `drop_x` with the player standing at `px`. Toward the player
/// is the default; equal x faces left.
static float opponent_facing(DrillFacing facing, float drop_x, float px)
{
    float toward = px > drop_x ? 1.0F : -1.0F;

    switch (facing) {
    case DrillFacing_Away:
        return -toward;
    case DrillFacing_Left:
        return -1.0F;
    case DrillFacing_Right:
        return 1.0F;
    default:
        return toward;
    }
}

/// Reset state: the player stands next to the opponent's drop point facing it
/// at 0% (only if the drill resets the player, else they stay where they
/// are), and the opponent is respawned over the drop point at 0%, facing as
/// the drill says. Then the drop: the opponent enters tumble from rest.
static void do_rep_setup(const Layout* layout)
{
    Vec3 start = layout->start;
    Vec3 drop = layout->landing;
    HSD_GObj* opp;
    const Drill* drill = training_drill_current();
    float drop_height = training_drill_pick_height(drill);
    bool place_player = drill->reset_player || !player_placed;

    reset_rep_data(layout->landing.y);
    drop.y += drop_height;

    if (place_player) {
        Player_SetFacingDirection(PLAYER_SLOT, 1.0F);
        Player_SetHUDDamage(PLAYER_SLOT, 0);
        teleport(PLAYER_SLOT, &start, true);
        player_placed = true;
    }

    Player_SetFacingDirection(OPPONENT_SLOT,
                              opponent_facing(drill->opponent_facing, drop.x,
                                              player_x()));
    Player_SetHUDDamage(OPPONENT_SLOT, 0);
    teleport(OPPONENT_SLOT, &drop, false);
    rep.option = choose_tech_option();
    REP_LOG_START();
    LOG_LAYOUT(layout);
    REP_LOG_WITH("drop", " height=%d option=%s opp_facing=%d player_x=%d",
                 (int) drop_height, tech_option_name(rep.option),
                 (int) GET_FIGHTER(Player_GetEntity(OPPONENT_SLOT))->facing_dir,
                 (int) player_x());

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
    rep.frames.window_open = rep.rep_frames;
    rep.tech = tech_results[tech_result(fp, player_x())].tech;
    REP_LOG("landing");
#ifdef AUTHORING_BUILD
    {
        float px = player_x();
        TechResult result = tech_result(fp, px);

        REP_LOG_WITH("tech",
                     " option=%s result_state=%s forced_ok=%s player_side=%s",
                     tech_option_name(rep.option),
                     tech_results[result].log_name,
                     result_matches(rep.option, result) ? "true" : "false",
                     side_of_player(fp->cur_pos.x, fp->facing_dir, px) < 0
                         ? "left"
                         : "right");
    }
#endif
}

/* A missed tech's window spans several motions (bounce, lying, getup), so
 * before the getup its last vulnerable frame cannot be read from the
 * animation. Its length (last vulnerable frame - window_open) is the same
 * every time for one opponent character, since the getup input comes at a
 * fixed time (MISSED_TECH_DOWN_TIME): it is learnt from every missed tech that
 * reaches its getup, success or failure, and kept for the character it was
 * learnt from (across drill events). NO_FRAME until then. */
static int missed_vuln_len = NO_FRAME;
static int missed_vuln_len_kind;

/// True for the motions that run to the opponent's first actionable frame in
/// one animation: the techs on the floor and the getup after a missed tech.
static bool runs_to_actionable(int motion)
{
    switch (motion) {
    case ftCo_MS_Passive:
    case ftCo_MS_PassiveStandF:
    case ftCo_MS_PassiveStandB:
    case ftCo_MS_DownStandU:
    case ftCo_MS_DownStandD:
        return true;
    default:
        return false;
    }
}

/// Predicts the window's last vulnerable frame from the animation the opponent
/// is in: the frame before its animation ends at its current rate (the end is
/// the actionable frame, which is not in the window). A hit replaces the
/// animation, so the prediction of the frame before the hit is the one a
/// success uses. A missed tech before its getup uses the learnt length
/// (missed_vuln_len); other motions leave the prediction alone.
static void predict_vuln_to(Fighter* fp)
{
    if (runs_to_actionable(fp->motion_id)) {
        float left = ftAnim_8006F484(fp->gobj) - ftAnim_8006F3DC(fp->gobj);
        int frames = 0;

        if (fp->frame_speed_mul > 0.0F && left > 0.0F) {
            frames = (int) (left / fp->frame_speed_mul + 0.999F);
        }
        /* On a failure this equals the measured vuln_to (the outcome line's
         * pred_vuln_to). */
        rep.frames.predicted_vuln_to = rep.rep_frames + frames - 1;
        if (rep.tech == RepTech_Missed) {
            missed_vuln_len =
                rep.frames.predicted_vuln_to - rep.frames.window_open;
            missed_vuln_len_kind = fp->kind;
        }
    } else if (rep.tech == RepTech_Missed && missed_vuln_len != NO_FRAME &&
               missed_vuln_len_kind == fp->kind &&
               rep.frames.predicted_vuln_to == NO_FRAME)
    {
        rep.frames.predicted_vuln_to =
            rep.frames.window_open + missed_vuln_len;
    }
}

/// True when the player has a hitbox out this frame. Grab boxes are hitboxes
/// too (the catch element), so a grab counts.
static bool player_attack_out(void)
{
    Fighter* fp = GET_FIGHTER(Player_GetEntity(PLAYER_SLOT));
    int i;

    for (i = 0; i < ARRAY_SIZE(fp->x914); i++) {
        if (fp->x914[i].state != HitCapsule_Disabled) {
            return true;
        }
    }
    return false;
}

/// On the first vulnerable or actionable frame after an early attack: the
/// attack was this many frames early. A later early attack replaces it.
static void resolve_early_attack(void)
{
    if (rep.early_pending) {
        rep.frames.early_frames =
            rep.rep_frames - rep.frames.early_attack_frame;
        rep.early_pending = false;
    }
}

/// Measures the current frame of the punish window. True when the window
/// closed this frame (the opponent is actionable, back in Wait) or the rep
/// timed out. Expects the frame to be counted already.
static bool measure_window(Fighter* fp)
{
    if (fp->motion_id == ftCo_MS_Wait) {
        /* The actionable frame closes the window and is not part of it. */
        rep.frames.actionable = rep.rep_frames;
        resolve_early_attack();
        REP_LOG("actionable");
        return true;
    }
    if (opponent_vulnerable(fp)) {
        if (rep.frames.vuln_from == NO_FRAME) {
            rep.frames.vuln_from = rep.rep_frames;
        }
        rep.frames.vuln_to = rep.rep_frames;
        resolve_early_attack();
    } else if (player_attack_out()) {
        /* Intangible: an attack out now whiffs (an early attack), however
         * many vulnerable frames came before (a missed tech's getup). */
        rep.frames.early_attack_frame = rep.rep_frames;
        rep.early_pending = true;
    }
    predict_vuln_to(fp);
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

/// The rep's timing class (rep_result.h), from the frames it measured.
static RepTiming classify_timing(void)
{
    return training_rep_classify(&rep.frames);
}

#ifdef AUTHORING_BUILD
/// Writes the rep's outcome line. The window fields are -1 when the window
/// never opened or closed; hit_frame is -1 and hit_kind=none for no punish.
/// `timing` is the timing class (training_rep_timing_log_value).
static void log_outcome(RepOutcome outcome, const RepTiming* timing)
{
    char value[REP_TIMING_LOG_MAX];

    training_rep_timing_log_value(value, timing);
    REP_LOG_WITH("outcome",
                 " outcome=%s window_open=%d vuln_from=%d vuln_to=%d "
                 "actionable=%d hit_frame=%d hit_kind=%s timing=%s"
                 " pred_vuln_to=%d",
                 training_rep_outcome_log_name(outcome),
                 rep.frames.window_open, rep.frames.vuln_from,
                 rep.frames.vuln_to, rep.frames.actionable,
                 rep.frames.hit.frame, hit_kind_name(rep.frames.hit.kind),
                 value, rep.frames.predicted_vuln_to);
}
#else
#define log_outcome(outcome, timing) ((void) 0)
#endif

/// Ends the rep with `outcome`: classifies its timing, writes the outcome line
/// and shows the result for the pause.
static void report_outcome(RepOutcome outcome)
{
    RepTiming timing = classify_timing();

    log_outcome(outcome, &timing);
    training_rep_result_show(rep.tech, outcome, &timing);
}

#ifdef AUTHORING_BUILD
/// Writes the rep's `late` line: a hit or grab after a failure, during the
/// pause. Its `timing` (late_Nf) is the rep's final timing class and replaces
/// the outcome line's; the outcome line itself stays as written.
static void log_late(const RepTiming* timing)
{
    char value[REP_TIMING_LOG_MAX];

    training_rep_timing_log_value(value, timing);
    REP_LOG_WITH("late", " actionable=%d hit_frame=%d hit_kind=%s timing=%s",
                 rep.frames.actionable, rep.frames.late_hit.frame,
                 hit_kind_name(rep.frames.late_hit.kind), value);
}
#else
#define log_late(timing) ((void) 0)
#endif

/// Records `hit` as the rep's late hit if it is one: the rep failed (the
/// opponent became actionable unpunished) and has no late hit yet. True when
/// recorded.
static bool record_late_hit(HitKind hit)
{
    if (hit == HitKind_None || rep.frames.actionable == NO_FRAME ||
        rep.frames.hit.frame != NO_FRAME ||
        rep.frames.late_hit.frame != NO_FRAME)
    {
        return false;
    }
    rep.frames.late_hit.frame = rep.rep_frames;
    rep.frames.late_hit.kind = hit;
    return true;
}

/// A failed rep turned late: writes the late line and shows the late result
/// for the rest of the pause. Still a failure, and the score is unchanged.
static void report_late(void)
{
    RepTiming timing = classify_timing();

    log_late(&timing);
    training_rep_result_show(rep.tech, RepOutcome_Failure, &timing);
}

/* The on-screen notice (invalid layout): canvas position and size of its lines. */
#define NOTICE_X 60.0F
#define NOTICE_Y 130.0F
#define NOTICE_LINE_HEIGHT 44.0F
#define NOTICE_SCALE 0.9F

static void add_notice_line(HSD_Text* text, int row, const char* ascii)
{
    training_text_add(text, NOTICE_X, NOTICE_Y + NOTICE_LINE_HEIGHT * row,
                      NOTICE_SCALE, NOTICE_SCALE, ascii);
}

/* The on-screen notice of the moment (invalid layout), hidden when a drill
 * starts. */
static HSD_Text* notice_text;

static void hide_notice(void)
{
    if (notice_text != NULL) {
        training_text_set_visible(notice_text, false);
        notice_text = NULL;
    }
}

/// The drill's layout cannot be used (no floor below one of its points, or no
/// usable drop heights): say so in the rep log and on screen, once. The
/// caller then stops the drill.
static void report_invalid_layout(LayoutStatus status)
{
    const Drill* drill = training_drill_current();
    HSD_Text* text;
    const char* problem;

    switch (status) {
    case Layout_NoLandingFloor:
        problem = "No floor below landing";
        break;
    case Layout_BadHeights:
        problem = "Bad drop heights";
        break;
    default:
        problem = "No floor below start";
        break;
    }
    LOG_INVALID_LAYOUT(drill, status);
    text = training_text_create();
    notice_text = text;
    add_notice_line(text, 0, "Invalid layout");
    add_notice_line(text, 1, problem);
    add_notice_line(text, 2, drill->name);
}

/// Hides or shows the opponent's model and shadow. The game clears both flags
/// on every motion state change, so while hidden this is called every frame.
static void set_opponent_hidden(Fighter* fp, bool hidden)
{
    fp->invisible = hidden;
    fp->x221E_b1 = hidden; /* casts no shadow (ftLib_CastsShadow) */
}

/// Shows the opponent again after a vanish and logs the hidden frames.
static void end_vanish(Fighter* fp)
{
    if (!rep.vanish_hidden) {
        return;
    }
    set_opponent_hidden(fp, false);
    rep.vanish_hidden = false;
    REP_LOG_WITH("vanish", " vanish_from=%d vanish_to=%d", rep.vanish_from,
                 rep.rep_frames - 1);
}

/// The vanish step, once a frame of the punish window has been measured: when
/// the drill vanishes, the tech is not a missed tech and the opponent has been
/// drawn for VANISH_VISIBLE_FRAMES, hide it until its first vulnerable frame.
static void update_vanish(Fighter* fp)
{
    bool hide;

    if (!training_drill_current()->vanish || rep.option == TechOption_Miss ||
        rep.frames.window_open == NO_FRAME)
    {
        return;
    }
    hide = rep.frames.vuln_from == NO_FRAME &&
           rep.rep_frames - rep.frames.window_open >= VANISH_VISIBLE_FRAMES;
    if (hide) {
        if (!rep.vanish_hidden) {
            rep.vanish_hidden = true;
            rep.vanish_from = rep.rep_frames;
        }
        set_opponent_hidden(fp, true);
    } else {
        end_vanish(fp);
    }
}

/// Per-frame check for the Drop and Wait states: ends the rep (into Pause) if
/// it was aborted, or if the player hit the opponent before it landed (void,
/// result shown), or moves to Landed, opening the punish window, when the
/// opponent lands (straight to Outcome, as a success, if it is hit that same
/// frame). True when the state changed.
static bool advance_if_landed_or_aborted(Fighter* fp, HitKind hit)
{
    if (rep_aborted(fp)) {
        set_state(RepState_Pause);
        return true;
    }
    if (in_tech_states(fp->motion_id)) {
        open_window(fp);
        measure_window(fp);
        update_vanish(fp);
        if (hit != HitKind_None) {
            record_hit(hit);
        }
        set_state(hit != HitKind_None ? RepState_Outcome : RepState_Landed);
        return true;
    }
    if (hit != HitKind_None) {
        record_hit(hit);
        rep.frames.voided = true;
        rep.tech = training_rep_tech_of_option(rep.option);
        report_outcome(RepOutcome_Void);
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
    int window_end = rep.frames.actionable;

    if (window_end == NO_FRAME && rep.frames.predicted_vuln_to != NO_FRAME) {
        window_end = rep.frames.predicted_vuln_to + 1;
    }
    if (window_end != NO_FRAME && rep.frames.window_open != NO_FRAME) {
        training_punish_window_record(
            GET_FIGHTER(Player_GetEntity(OPPONENT_SLOT))->kind, rep.tech,
            window_end - rep.frames.window_open);
    }
    if (rep.frames.hit.frame != NO_FRAME) {
        training_score_record(rep.tech, true);
        report_outcome(RepOutcome_Success);
    } else if (rep.frames.actionable != NO_FRAME) {
        training_score_record(rep.tech, false);
        rep.scored_failure = true;
        report_outcome(RepOutcome_Failure);
    }
}

/* Frames the current drill has run (not counting settle or the summary), for
 * a time-limited drill's clock. */
static int drill_frames;
#define DRILL_FPS 60

/// True when the current drill's length is reached: a rep-count drill has
/// counted that many reps (voids and abandoned reps are not counted); a
/// time-limited drill has run its length in seconds. Only checked between
/// reps, so the rep in progress when time runs out finishes and counts.
static bool drill_length_reached(void)
{
    const Drill* drill = training_drill_current();

    if (drill->length_kind == DrillLength_Time) {
        return drill_frames >= drill->length * DRILL_FPS;
    }
    return training_score_counted() >= drill->length;
}

/* The summary's menu (Restart, Next drill, Event list), shown beside the
 * summary text while the rep loop is in RepState_Summary. A on an item sets
 * `summary_choice`; the summary state acts on it. */
typedef enum SummaryChoice {
    SummaryChoice_None,
    SummaryChoice_Restart,
    SummaryChoice_NextDrill,
    SummaryChoice_EventList,
} SummaryChoice;

static SummaryChoice summary_choice;

static void choose_summary_restart(void)
{
    summary_choice = SummaryChoice_Restart;
}

static void choose_summary_next_drill(void)
{
    summary_choice = SummaryChoice_NextDrill;
}

static void choose_summary_event_list(void)
{
    summary_choice = SummaryChoice_EventList;
}

/* The summary menu's items, in order. */
enum {
    SUMMARY_ITEM_RESTART,
    SUMMARY_ITEM_NEXT_DRILL,
    SUMMARY_ITEM_EVENT_LIST,
};

static TrainingMenuItem summary_items[] = {
    { "Restart", "Play it again", true, choose_summary_restart },
    { "Next drill", "Next in list", true, choose_summary_next_drill },
    { "Event list", "Pick a drill", true, choose_summary_event_list },
};

/* Left of the summary text (score.c). */
#define SUMMARY_MENU_X 30.0F
#define SUMMARY_MENU_Y 160.0F

static TrainingMenu summary_menu = { "Drill over", summary_items,
                                     ARRAY_SIZE(summary_items), 0, 0, NULL,
                                     SUMMARY_MENU_X, SUMMARY_MENU_Y };

/// Shows the summary's menu, with Next drill greyed out on the last drill.
static void show_summary_menu(void)
{
    summary_items[SUMMARY_ITEM_NEXT_DRILL].enabled =
        training_drill_has_next();
    summary_choice = SummaryChoice_None;
    /* The text from the last time stays; only the cursor starts over. */
    summary_menu.cursor = 0;
    summary_menu.top = 0;
    training_menu_show(&summary_menu);
}

/// Starts the current drill from rep 1, clearing everything of the previous
/// run: the notice, summary and rep result, the score, the drill clock, the
/// player's placement and any vanish in progress (the opponent is shown
/// again). Restarting a finished drill and switching drills both start here.
static void start_drill(Fighter* opp_fp)
{
    hide_notice();
    training_rep_result_hide();
    training_punish_window_panel_show();
    training_summary_hide();
    training_menu_hide(&summary_menu);
    training_score_clear();
    end_vanish(opp_fp);
    player_placed = false;
    drill_frames = 0;
    /* A tech press due this frame belongs to the run being thrown away. */
    rep.press_pending = false;
    REP_LOG_RESTART();
    log_drill_start(training_drill_current());
    set_state(RepState_Reset);
}

/// Restarts the drill in play from rep 1 (the pause menu's Restart drill).
void training_tech_chase_restart(void)
{
    HSD_GObj* opp = Player_GetEntity(OPPONENT_SLOT);

    if (opp != NULL) {
        start_drill(GET_FIGHTER(opp));
    }
}

/* Edit mode: the layout editor has the fighters as markers (see
 * tech_chase.h). Set from edit_begin to edit_end; the opponent's fed input
 * is neutral meanwhile. */
static bool edit_mode;

void training_tech_chase_edit_begin(void)
{
    HSD_GObj* opp = Player_GetEntity(OPPONENT_SLOT);

    edit_mode = true;
    /* A press due now belonged to the rep the markers displace. */
    rep.press_pending = false;
    training_rep_result_hide();
    if (opp != NULL) {
        end_vanish(GET_FIGHTER(opp));
    }
}

LayoutStatus training_tech_chase_edit_place(int height_index, Layout* out)
{
    const Drill* drill = training_drill_current();
    Layout layout;
    LayoutStatus status;
    Vec3 drop;

    status = training_layout_resolve(drill, player_reach + opponent_reach,
                                     &layout);
    if (status != Layout_Ok) {
        return status;
    }
    if (height_index < 0 || height_index >= drill->height_count) {
        height_index = 0;
    }
    drop = layout.landing;
    drop.y += drill->heights[height_index];
    Player_SetFacingDirection(PLAYER_SLOT, 1.0F);
    teleport(PLAYER_SLOT, &layout.start, true);
    Player_SetFacingDirection(OPPONENT_SLOT,
                              opponent_facing(drill->opponent_facing, drop.x,
                                              layout.start.x));
    teleport(OPPONENT_SLOT, &drop, false);
    if (out != NULL) {
        *out = layout;
    }
    return Layout_Ok;
}

void training_tech_chase_edit_end(bool layout_changed)
{
    edit_mode = false;
    if (layout_changed) {
        training_tech_chase_restart();
        return;
    }
    /* The same layout: carry on. A rep under way was displaced by the
     * markers, so it is dropped (not counted) and the next one starts. A
     * rep in its pause has already been scored and finishes it; settle, the
     * summary and an invalid layout stay as they were. */
    if (rep_state >= RepState_Reset && rep_state < RepState_Pause) {
        set_state(RepState_Reset);
    }
}

static void think(HSD_GObj* gobj)
{
    HSD_GObj* player = Player_GetEntity(PLAYER_SLOT);
    HSD_GObj* opp = Player_GetEntity(OPPONENT_SLOT);
    HitKind hit;
    bool window_closed;

    if (player == NULL || opp == NULL) {
        return;
    }
    rep_loop_ran = true;
    if (gm_GetDbPauseFlag(DB_PAUSE_FLAG_VS)) {
        return;
    }
    frames_in_state++;
    if (rep_state != RepState_Settle && rep_state != RepState_Summary) {
        drill_frames++;
    }
    REP_LOG_TICK();
    hit = poll_player_hit(GET_FIGHTER(opp));
    if (rep_state >= RepState_Drop && rep_state <= RepState_Pause) {
        /* A rep is under way: count its frame before anything measures it.
         * The count runs on through the outcome and the pause, so a late hit
         * is timed from the actionable frame. */
        rep.rep_frames++;
    }
    switch (rep_state) {
    case RepState_Settle:
        if (frames_in_state == 1) {
            training_score_show();
            training_rep_result_create();
            training_punish_window_panel_show();
        }
        /* A VS match (a drill event) holds the fighters through its
         * Ready/GO countdown: settle from the GO, or the first rep's tech
         * press is ignored. */
        if (GET_FIGHTER(player)->input_disabled ||
            GET_FIGHTER(opp)->input_disabled)
        {
            frames_in_state = 1;
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
        training_rep_result_hide();
        {
            Layout layout;
            LayoutStatus status;

            status = training_layout_resolve(
                training_drill_current(), player_reach + opponent_reach,
                &layout);

            if (status != Layout_Ok) {
                report_invalid_layout(status);
                set_state(RepState_Invalid);
                break;
            }
            do_rep_setup(&layout);
        }
        set_state(RepState_Drop);
        break;
    case RepState_Invalid:
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
        window_closed = measure_window(GET_FIGHTER(opp));
        update_vanish(GET_FIGHTER(opp));
        if (window_closed) {
            /* A hit on the actionable frame itself is late by 0 frames
             * (reported at the outcome). */
            record_late_hit(hit);
            set_state(RepState_Outcome);
        } else if (hit != HitKind_None) {
            /* Hit or grab inside the window: success, the rep ends at once. */
            record_hit(hit);
            set_state(RepState_Outcome);
        }
        break;
    case RepState_Outcome:
        /* A vanish never outlives the window (a timed-out window, or a hit
         * while hidden, shows the opponent here). */
        end_vanish(GET_FIGHTER(opp));
        finish_rep();
        /* The start-of-drill panel goes once a rep has measured a window. */
        training_punish_window_panel_hide();
        if (rep.frames.late_hit.frame != NO_FRAME || record_late_hit(hit)) {
            report_late();
        }
        set_state(RepState_Pause);
        break;
    case RepState_Pause:
        /* The rep's result stays on screen for the whole pause. After a
         * failure the first hit or grab in it makes the rep late. */
        if (record_late_hit(hit)) {
            report_late();
        }
        if (frames_in_state >= PAUSE_FRAMES) {
            /* The pause is over, so a failure can no longer turn late: its
             * timing class is final and counts towards the summary. */
            if (rep.scored_failure) {
                training_score_record_timing(rep.tech,
                                             classify_timing().kind);
            }
            if (drill_length_reached()) {
                /* The last rep has finished: stop dropping, show the
                 * summary. */
                training_rep_result_hide();
                log_drill_end(training_drill_current());
                training_summary_show(training_drill_current());
                show_summary_menu();
                set_state(RepState_Summary);
            } else {
                set_state(RepState_Reset);
            }
        }
        break;
    case RepState_Summary:
        /* The match is not paused here, so ending it works directly. B and
         * Start do nothing on the summary. */
        summary_choice = SummaryChoice_None;
        training_menu_update(&summary_menu, Player_GetPadPort(PLAYER_SLOT));
        switch (summary_choice) {
        case SummaryChoice_Restart:
            start_drill(GET_FIGHTER(opp));
            break;
        case SummaryChoice_NextDrill:
            training_menu_hide(&summary_menu);
            training_event_end_match(TrainingEventExit_NextDrill);
            break;
        case SummaryChoice_EventList:
            training_menu_hide(&summary_menu);
            training_event_end_match(TrainingEventExit_EventList);
            break;
        default:
            break;
        }
        break;
    }
}

/// Writes a rep log line `event` with the rep loop's progress (rep state,
/// frames into the rep, the drill clock and the score), then `extra`, so two
/// such lines show whether anything moved between them (a pause). Authoring
/// builds only; a no-op otherwise.
void training_tech_chase_log_progress(const char* event, const char* extra)
{
#ifdef AUTHORING_BUILD
    OSReport("[rep] event=%s rep=%d frame=%d state=%d frames_in_state=%d "
             "rep_frames=%d drill_frames=%d score=%d/%d%s\n",
             event, rep_number, rep_frame, rep_state, frames_in_state,
             rep.rep_frames, drill_frames, training_score_successes(),
             training_score_counted(), extra);
#endif
}

/// Called as a drill event's match start (src/training/event.c).
void training_tech_chase_init(void)
{
    set_state(RepState_Settle);
    drill_frames = 0;
    training_rep_result_init();
    notice_text = NULL;
    training_punish_window_panel_reset();
    prev_percent = 0.0F;
    prev_grabbed = false;
    player_placed = false;
    edit_mode = false;
    REP_LOG_RESET();
    log_drill_start(training_drill_current());
    reset_rep_data(0.0F);
    training_score_init();
    training_menu_reset(&summary_menu);
    HSD_GObj_SetupProc(GObj_Create(TECH_CHASE_GOBJ_CLASS,
                                   TECH_CHASE_GOBJ_PLINK,
                                   TECH_CHASE_GOBJ_PRIORITY),
                       (HSD_GObjEvent) think, TECH_CHASE_PROC_PRIORITY);
}

/// The per-option input step: feeds the opponent's controller for `option`
/// this frame, on top of the neutral input already set. `press` is true on the
/// frame the tech press is due (timed from the fall). Carrying out another
/// option means filling in its case; the rep loop does not change.
static void feed_option_input(Fighter* fp, TechOption option, bool press)
{
    switch (option) {
    case TechOption_InPlace:
        /* One L press with neutral stick; Fighter_procInput derives the
         * trigger and edge from the button bit. */
        if (press) {
            fp->cpu.buttons = HSD_PAD_L;
        }
        break;
    case TechOption_In:
    case TechOption_Away:
        /* The same press, with the stick held fully left or right from the
         * drop until the landing; the game turns it into its own forward/back
         * roll. The direction is re-read every frame, not latched: the game
         * reads the stick on the landing frame, so what counts is where the
         * player stands then, and a latched side mislabelled the roll whenever
         * the player crossed the opponent during the fall (measured). The
         * stick history is written as already held (as for a player who held
         * it into the hit), because a stick that starts or flips in the tumble
         * is a flick, which cancels the tech. Air drift is zeroed so the held
         * stick does not carry the opponent across the player before it
         * lands. */
        if (press) {
            fp->cpu.buttons = HSD_PAD_L;
        }
        if (!rep.seen_tech &&
            (rep_state == RepState_Drop || rep_state == RepState_Wait))
        {
            s8 stick =
                roll_stick_x(fp->cur_pos.x, fp->facing_dir, player_x(), option);

            fp->cpu.lstick.x = stick;
            fp->input.lstick[0].x = stick / 127.0F;
            fp->active_timer.lstick.x = 254;
            fp->self_vel.x = 0.0F;
        }
        break;
    case TechOption_Miss:
        /* No L/R and no A/B while falling, so the opponent bounces and lies
         * down; once it has lain for the down time, a plain getup (stick up,
         * held until it stands). */
        if (rep.frames.window_open != NO_FRAME &&
            rep.rep_frames - rep.frames.window_open >= MISSED_TECH_DOWN_TIME)
        {
            fp->cpu.lstick.y = 127;
        }
        break;
    default:
        break;
    }
}

/// Called from Fighter_procCpu after the CPU tick: replaces the opponent's
/// input with neutral so the AI does nothing (ADR-0004). Only in a drill
/// event: whether the rep loop ran this frame scopes it without a scene-exit hook (the
/// scene index is the mode's state index, not the scene kind, so it cannot).
void training_cpu_input(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);

    if (fp->player_idx != OPPONENT_SLOT || !rep_loop_ran) {
        return;
    }
    rep_loop_ran = false;
    fp->cpu.buttons = 0;
    fp->cpu.lstick.x = 0;
    fp->cpu.lstick.y = 0;
    fp->cpu.cstick.x = 0;
    fp->cpu.cstick.y = 0;
    fp->cpu.ltrigger = 0;
    fp->cpu.rtrigger = 0;
    if (edit_mode) {
        rep.press_pending = false;
        return;
    }
    feed_option_input(fp, rep.option, rep.press_pending);
    rep.press_pending = false;
}

#endif
