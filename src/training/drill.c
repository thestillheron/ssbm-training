#ifdef TRAINING_BUILD

#include "drill.h"

#include <sysdolphin/baselib/random.h>

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

static int current_index;

int training_drill_count(void)
{
    return DRILL_COUNT;
}

bool training_drill_has_next(void)
{
    return current_index + 1 < DRILL_COUNT;
}

const Drill* training_drill_at(int index)
{
    return &drills[index];
}

int training_drill_current_index(void)
{
    return current_index;
}

const Drill* training_drill_current(void)
{
    if (current_index < 0 || current_index >= DRILL_COUNT) {
        current_index = 0;
    }
    return &drills[current_index];
}

void training_drill_select(int index)
{
    if (index >= 0 && index < DRILL_COUNT) {
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
