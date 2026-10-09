#ifdef TRAINING_BUILD

#include "drill.h"

#include "layout.h"

#include <sysdolphin/baselib/random.h>

#ifdef AUTHORING_BUILD
#include <dolphin/os.h>
#endif

#include <stdio.h>

/* dev.py --tech-pool compiles a pool mask in for acceptance scenarios; it
 * replaces the pool of every drill. Otherwise each drill's own pool is used. */
#ifdef TECH_OPTION_POOL
#define POOL_FOR(drill_pool) TECH_OPTION_POOL
#else
#define POOL_FOR(drill_pool) (drill_pool)
#endif

/* Fields in the order of struct Drill (drill.h); the compiler has no
 * designated initialisers. */
static const Drill drills[] = {
    {
        "Tech in place",
        "Opponent drops and techs in place.",
        DRILL_STAGE_FINAL_DESTINATION,
        0.0F,
        0.0F,
        DrillHeight_List,
        1,
        { 40.0F },
        0.0F,
        0.0F,
        DrillFacing_TowardPlayer,
        true,
        POOL_FOR(TECH_OPTION_BIT(TechOption_InPlace)),
        false,
        DrillLength_Reps,
        10,
        7,
    },
#ifdef AUTHORING_BUILD
    /* Test drills, for the acceptance scenarios (chosen from the event
     * list by position). */
    {
        "Test: three heights",
        "Drop height picked from a list of three.",
        DRILL_STAGE_FINAL_DESTINATION,
        0.0F,
        0.0F,
        DrillHeight_List,
        3,
        { 30.0F, 60.0F, 90.0F },
        0.0F,
        0.0F,
        DrillFacing_TowardPlayer,
        true,
        POOL_FOR(TECH_OPTION_BIT(TechOption_InPlace)),
        false,
        DrillLength_Reps,
        10,
        7,
    },
    {
        "Test: height range",
        "Drop height picked uniformly from 30 to 90.",
        DRILL_STAGE_FINAL_DESTINATION,
        0.0F,
        0.0F,
        DrillHeight_Range,
        2,
        { 30.0F, 90.0F },
        0.0F,
        0.0F,
        DrillFacing_TowardPlayer,
        true,
        POOL_FOR(TECH_OPTION_BIT(TechOption_InPlace)),
        false,
        DrillLength_Reps,
        10,
        7,
    },
    {
        "Test: platform",
        "Opponent lands on the top platform; player on the floor.",
        DRILL_STAGE_BATTLEFIELD,
        0.0F,
        54.0F,
        DrillHeight_List,
        1,
        { 40.0F },
        0.0F,
        0.0F, /* no offset: the player stands under the platform */
        DrillFacing_TowardPlayer,
        true,
        POOL_FOR(TECH_OPTION_BIT(TechOption_InPlace)),
        false,
        DrillLength_Reps,
        10,
        7,
    },
    {
        "Test: invalid layout",
        "Landing point far off the stage: no floor below it.",
        DRILL_STAGE_FINAL_DESTINATION,
        5000.0F,
        0.0F,
        DrillHeight_List,
        1,
        { 40.0F },
        0.0F,
        0.0F,
        DrillFacing_TowardPlayer,
        true,
        POOL_FOR(TECH_OPTION_BIT(TechOption_InPlace)),
        false,
        DrillLength_Reps,
        10,
        7,
    },
    {
        "Test: vanish",
        "Authoring test: the opponent vanishes mid-tech.",
        DRILL_STAGE_FINAL_DESTINATION,
        0.0F,
        0.0F,
        DrillHeight_List,
        1,
        { 40.0F },
        0.0F,
        0.0F,
        DrillFacing_TowardPlayer,
        true,
        POOL_FOR(TECH_OPTION_BIT(TechOption_InPlace) | TECH_OPTION_BIT(TechOption_In) | TECH_OPTION_BIT(TechOption_Away) | TECH_OPTION_BIT(TechOption_Miss)),
        true,
        DrillLength_Reps,
        10,
        7,
    },
    {
        "Test: no reset",
        "Authoring test: the player is not reset between reps.",
        DRILL_STAGE_FINAL_DESTINATION,
        0.0F,
        0.0F,
        DrillHeight_List,
        1,
        { 40.0F },
        0.0F,
        0.0F,
        DrillFacing_TowardPlayer,
        false,
        POOL_FOR(TECH_OPTION_BIT(TechOption_InPlace)),
        false,
        DrillLength_Reps,
        10,
        7,
    },
    {
        "Test: face away",
        "Authoring test: the opponent faces away from the player.",
        DRILL_STAGE_FINAL_DESTINATION,
        0.0F,
        0.0F,
        DrillHeight_List,
        1,
        { 40.0F },
        0.0F,
        0.0F,
        DrillFacing_Away,
        true,
        POOL_FOR(TECH_OPTION_BIT(TechOption_InPlace)),
        false,
        DrillLength_Reps,
        10,
        7,
    },
    {
        "Test: face left",
        "Authoring test: the opponent faces left.",
        DRILL_STAGE_FINAL_DESTINATION,
        0.0F,
        0.0F,
        DrillHeight_List,
        1,
        { 40.0F },
        0.0F,
        0.0F,
        DrillFacing_Left,
        true,
        POOL_FOR(TECH_OPTION_BIT(TechOption_InPlace)),
        false,
        DrillLength_Reps,
        10,
        7,
    },
    {
        "Test: face right",
        "Authoring test: the opponent faces right.",
        DRILL_STAGE_FINAL_DESTINATION,
        0.0F,
        0.0F,
        DrillHeight_List,
        1,
        { 40.0F },
        0.0F,
        0.0F,
        DrillFacing_Right,
        true,
        POOL_FOR(TECH_OPTION_BIT(TechOption_InPlace)),
        false,
        DrillLength_Reps,
        10,
        7,
    },
    {
        "Test: time limit",
        "Ends after 15 seconds; the rep in progress finishes.",
        DRILL_STAGE_FINAL_DESTINATION,
        0.0F,
        0.0F,
        DrillHeight_List,
        1,
        { 40.0F },
        0.0F,
        0.0F,
        DrillFacing_TowardPlayer,
        true,
        POOL_FOR(TECH_OPTION_BIT(TechOption_InPlace)),
        false,
        DrillLength_Time,
        15,
        5,
    },
    {
        "Test: no heights",
        "Invalid drill: a list of no drop heights.",
        DRILL_STAGE_FINAL_DESTINATION,
        0.0F,
        0.0F,
        DrillHeight_List,
        0,
        { 0.0F },
        0.0F,
        0.0F,
        DrillFacing_TowardPlayer,
        true,
        POOL_FOR(TECH_OPTION_BIT(TechOption_InPlace)),
        false,
        DrillLength_Reps,
        10,
        7,
    },
    /* One-rep drills, for the summary menu's Next drill: from the first to
     * the second changes the stage, and the second is the last drill. */
    {
        "Test: one rep",
        "Authoring test: ends after one rep.",
        DRILL_STAGE_FINAL_DESTINATION,
        0.0F,
        0.0F,
        DrillHeight_List,
        1,
        { 40.0F },
        0.0F,
        0.0F,
        DrillFacing_TowardPlayer,
        true,
        POOL_FOR(TECH_OPTION_BIT(TechOption_InPlace)),
        false,
        DrillLength_Reps,
        1,
        1,
    },
    {
        "Test: one rep, last",
        "Authoring test: the last drill, on Battlefield.",
        DRILL_STAGE_BATTLEFIELD,
        0.0F,
        54.0F,
        DrillHeight_List,
        1,
        { 40.0F },
        0.0F,
        0.0F,
        DrillFacing_TowardPlayer,
        true,
        POOL_FOR(TECH_OPTION_BIT(TechOption_InPlace)),
        false,
        DrillLength_Reps,
        1,
        1,
    },
#endif
};

#define DRILL_COUNT ((int) (sizeof(drills) / sizeof(drills[0])))

/* The Custom drill: one session-scoped record of the same type, listed after
 * the table. Its stage is the stage select's pick, and its layout is fitted
 * to that stage (training_drill_custom_fit). */
#define CUSTOM_BASE_HEIGHT 40.0F
#define CUSTOM_REPS 10
static Drill custom = {
    "Custom drill",
    "Your own drill on any stage.",
    DRILL_STAGE_FINAL_DESTINATION,
    0.0F,
    0.0F,
    DrillHeight_List,
    1,
    { CUSTOM_BASE_HEIGHT },
    0.0F,
    0.0F,
    DrillFacing_TowardPlayer,
    true,
    POOL_FOR(TECH_OPTION_BIT(TechOption_InPlace) |
             TECH_OPTION_BIT(TechOption_In) |
             TECH_OPTION_BIT(TechOption_Away) |
             TECH_OPTION_BIT(TechOption_Miss)),
    false,
    DrillLength_Reps,
    CUSTOM_REPS,
    0, /* no pass mark */
};
/* The stage the custom layout was last fitted to; -1 before the first. */
static int custom_fitted_stage = -1;

static int current_index;

/* The session copy of the current drill: what the rep loop runs and the
 * layout editor edits. Copied from the table when a different drill is
 * selected (or on first use); until then edits survive restarts, retries and
 * re-entering the same drill. The table itself is never written. */
static Drill session;
static int session_index = -1;

int training_drill_count(void)
{
    return DRILL_COUNT;
}

bool training_drill_has_next(void)
{
    return current_index + 1 < DRILL_COUNT;
}

int training_drill_event_count(void)
{
    return DRILL_COUNT + 1;
}

int training_drill_custom_index(void)
{
    return DRILL_COUNT;
}

bool training_drill_is_custom(int index)
{
    return index == DRILL_COUNT;
}

void training_drill_custom_set_stage(int stage)
{
    custom.stage = stage;
}

/// Puts the custom layout at the defaults: landing point (`x`, `y`) and the
/// default player start. The drop heights are a setting and are kept.
static void set_default_custom_layout(float x, float y)
{
    custom.landing_x = x;
    custom.landing_y = y;
    custom.start_dx = 0.0F;
    custom.start_dy = 0.0F;
}

void training_drill_custom_fit(void)
{
    Vec3 floor;
    float x;
    float y;

    if (!training_drill_is_custom(current_index) || custom_fitted_stage == custom.stage) {
        return;
    }
    if (custom_fitted_stage >= 0 &&
        training_layout_landing_floor(custom.landing_x, custom.landing_y,
                                      &floor))
    {
        custom_fitted_stage = custom.stage;
        return;
    }
    if (training_layout_default_landing(&x, &y)) {
        set_default_custom_layout(x, y);
        custom_fitted_stage = custom.stage;
    }
}

Drill* training_drill_custom(void)
{
    return &custom;
}

const Drill* training_drill_at(int index)
{
    if (training_drill_is_custom(index)) {
        return &custom;
    }
    return &drills[index];
}

bool training_drill_has_pass_mark(const Drill* drill)
{
    return drill->pass_mark != 0;
}

int training_drill_current_index(void)
{
    return current_index;
}

Drill* training_drill_session(void)
{
    if (training_drill_is_custom(current_index)) {
        return &custom;
    }
    if (current_index < 0 || current_index >= DRILL_COUNT) {
        current_index = 0;
    }
    if (session_index != current_index) {
        session = drills[current_index];
        session_index = current_index;
    }
    return &session;
}

void training_drill_revert_layout(void)
{
    Drill* drill = training_drill_session();
    const Drill* compiled;
    int i;

    if (training_drill_is_custom(current_index)) {
        float x = 0.0F;
        float y = 0.0F;

        training_layout_default_landing(&x, &y);
        set_default_custom_layout(x, y);
        return;
    }
    compiled = &drills[session_index];

    drill->landing_x = compiled->landing_x;
    drill->landing_y = compiled->landing_y;
    drill->height_kind = compiled->height_kind;
    drill->height_count = compiled->height_count;
    for (i = 0; i < DRILL_MAX_HEIGHTS; i++) {
        drill->heights[i] = compiled->heights[i];
    }
    drill->start_dx = compiled->start_dx;
    drill->start_dy = compiled->start_dy;
}

bool training_drill_same_layout(const Drill* a, const Drill* b)
{
    int i;

    if (a->landing_x != b->landing_x || a->landing_y != b->landing_y ||
        a->height_kind != b->height_kind ||
        a->height_count != b->height_count || a->start_dx != b->start_dx ||
        a->start_dy != b->start_dy)
    {
        return false;
    }
    for (i = 0; i < a->height_count && i < DRILL_MAX_HEIGHTS; i++) {
        if (a->heights[i] != b->heights[i]) {
            return false;
        }
    }
    return true;
}

/// `value` in whole tenths, rounded to the nearest.
static int to_tenths(float value)
{
    return (int) (value * 10.0F + (value < 0.0F ? -0.5F : 0.5F));
}

float training_drill_round_tenths(float value)
{
    return (float) to_tenths(value) / 10.0F;
}

char* training_drill_format_tenths(char* out, float value)
{
    int tenths = to_tenths(value);

    if (tenths < 0) {
        *out++ = '-';
        tenths = -tenths;
    }
    return out + sprintf(out, "%d.%d", tenths / 10, tenths % 10);
}

#ifdef AUTHORING_BUILD
/* Room for the longest Print layout payload: six floats, four heights and
 * the height kind's name, with their separators. */
#define LAYOUT_LINE_MAX 192

/// Appends `value` as a C float literal rounded to tenths ("-7.5F").
static char* print_float(char* p, float value)
{
    p = training_drill_format_tenths(p, value);
    *p++ = 'F';
    *p = 0;
    return p;
}

void training_drill_print_layout(void)
{
    const Drill* drill = training_drill_current();
    char line[LAYOUT_LINE_MAX];
    char* p = line;
    int i;

    p = print_float(p, drill->landing_x);
    p += sprintf(p, ", ");
    p = print_float(p, drill->landing_y);
    p += sprintf(p, ", %s, %d, { ",
                 drill->height_kind == DrillHeight_Range ? "DrillHeight_Range"
                                                         : "DrillHeight_List",
                 drill->height_count);
    for (i = 0; i < drill->height_count; i++) {
        if (i > 0) {
            p += sprintf(p, ", ");
        }
        p = print_float(p, drill->heights[i]);
    }
    p += sprintf(p, " }, ");
    p = print_float(p, drill->start_dx);
    p += sprintf(p, ", ");
    p = print_float(p, drill->start_dy);
    OSReport("[rep] layout: %s,\n", line);
}
#endif

const Drill* training_drill_current(void)
{
    return training_drill_session();
}

void training_drill_select(int index)
{
    if (index >= 0 && index <= DRILL_COUNT) {
        current_index = index;
    }
}

float training_drill_pick_height(const Drill* drill)
{
    if (drill->height_kind == DrillHeight_Range) {
        return drill->heights[0] +
               HSD_Randf() * (drill->heights[1] - drill->heights[0]);
    }
    if (drill->height_count <= 1) {
        return drill->heights[0];
    }
    return drill->heights[HSD_Randi(drill->height_count)];
}

#endif
