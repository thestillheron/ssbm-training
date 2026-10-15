#ifdef TRAINING_BUILD

#include "event.h"

#include <melee/gm/gm_1601.h>
#include <melee/gm/gm_1A3F.h>
#include <melee/gm/gm_1B03.h>
#include <melee/gm/gmmain_lib.h>
#include <melee/gm/gmvs.h>
#include <melee/gm/types.h>
#include <melee/lb/lbaudio_ax.h>
#include <melee/lb/lbdvd.h>
#include <melee/lb/types.h>
#include <melee/mn/types.h>

#include <dolphin/os.h>

#include "drill.h"
#include "pause.h"
#include "tech_chase.h"

/* Training Mode's player + CPU character select. */
#define CSS_TYPE_PLAYER_AND_CPU 0x17
#define EVENT_COUNT 51
#define CPU_LEVEL 9
/* Event Match's scene states (gm_Mode_Event_States, gmevent.c). */
#define STATE_CSS 0
#define STATE_VS 1
#define STATE_SSS 2


/* Set by training_event_end_match, read by onExitVs. */
static bool exit_requested;
static TrainingEventExit requested_exit;

static struct EventData* event_data(void)
{
    return &gmMainLib_804D3EE0->vs.unk_530;
}

int training_event_drill_index(void)
{
    /* The event list shows the drill table (event_list.c), so a position on
     * it is a drill's index in the table. */
    int index = event_data()->unk_535;

    /* The Custom drill's index is past the list's last row. */
    if (index <= training_drill_custom_index()) {
        return index;
    }
    return -1;
}

bool training_event_drill_in_play(void)
{
    return gm_GetCurrentGameMode() == GM_EVENT &&
           training_event_drill_index() >= 0;
}

bool training_event_cstick_enabled(void)
{
    return training_event_drill_in_play();
}

/* The characters picked on the drill events' character select, kept across
 * drills, retries and visits to the event list. The CPU starts as the
 * character Training Mode's own select would give it. */
static bool picks_set;
static s8 player_ckind = ChKind_None;
static u8 player_color;
static u8 player_nametag;
static s8 cpu_ckind;
static s8 cpu_color;
static s8 cpu_nametag;

static void init_picks(void)
{
    VsModeData* training;

    if (picks_set) {
        return;
    }
    training = &gmMainLib_804D3EE0->modes.table[GmVsMode_Training];
    player_ckind = event_data()->x2;
    player_color = event_data()->x3;
    player_nametag = event_data()->nametag;
    cpu_ckind = training->start.players[1].ckind;
    cpu_color = training->start.players[1].color;
    cpu_nametag = training->start.players[1].nametag;
    picks_set = true;
}

#ifdef AUTHORING_BUILD
/// One rep log line with Event Match's save fields (cleared bits and the
/// best scores), so a scenario can check that a drill event leaves them as
/// they were.
static void log_event_save(const char* when)
{
    u32 cleared_hi = 0;
    u32 cleared_lo = 0;
    s32 best_sum = 0;
    int i;

    for (i = 0; i < EVENT_COUNT; i++) {
        if (gmMainLib_8015CEFC(i)) {
            if (i < 32) {
                cleared_lo |= 1U << i;
            } else {
                cleared_hi |= 1U << (i - 32);
            }
        }
        best_sum += gmMainLib_8015CF5C(i);
    }
    OSReport("[rep] event=event_save when=%s event=%d cleared=%08x%08x "
             "best0=%d best_sum=%d\n",
             when, event_data()->unk_535, cleared_hi, cleared_lo,
             gmMainLib_8015CF5C(0), best_sum);
}
#define LOG_EVENT_SAVE(when) log_event_save(when)

/// The rep log's record of a drill chosen on the event list.
static void log_menu_choice(int index)
{
    OSReport("[rep] event=menu menu=event_list choice=drill drill=%d "
             "name=\"%s\"\n",
             index + 1, training_drill_at(index)->name);
}
#define LOG_MENU_CHOICE(index) log_menu_choice(index)
#else
#define LOG_EVENT_SAVE(when) ((void) 0)
#define LOG_MENU_CHOICE(index) ((void) 0)
#endif

void training_event_on_load(void)
{
    int index = training_event_drill_index();

    if (index < 0) {
        return;
    }
    /* Event Match loads each time an event is chosen on the list. */
    LOG_MENU_CHOICE(index);
    gm_SetGameModeStateId(0);
}

bool training_event_enter_css(CSSData* css)
{
    u8 port = event_data()->x6;

    if (training_event_drill_index() < 0) {
        return false;
    }
    init_picks();
    gm_801B06B0(css, CSS_TYPE_PLAYER_AND_CPU, player_ckind, 1, player_color,
                player_nametag, CPU_LEVEL, port);
    gm_801B07B4(css, cpu_ckind, 1, cpu_color, cpu_nametag, 0, port);
    return true;
}

bool training_event_exit_css(CSSData* css)
{
    if (training_event_drill_index() < 0) {
        return false;
    }
    gm_801B0730(css, &player_ckind, NULL, &player_color, &player_nametag,
                NULL);
    gm_801B07E8(css, &cpu_ckind, NULL, &cpu_color, &cpu_nametag, NULL);
    if (training_drill_is_custom(training_event_drill_index())) {
        /* The Custom drill picks its stage next; the curriculum's drills
         * bring their own. */
        gm_SetNextGameModeStateId(STATE_SSS);
    }
    return true;
}

/* The Custom drill's stage select (Training Mode's, which is the retail
 * screen with the stage list). It is state 2, after the match: only the
 * Custom drill's character select routes to it. */
SSSData training_event_sss_data;

void training_event_enter_sss(GameModeState* state)
{
    SSSData* sss = gm_GetGameModeStateEnterData(state);

    sss->x1 = 0;
    sss->force_stage_id = -1;
    sss->unk_stage = 0;
}

void training_event_exit_sss(GameModeState* state)
{
    SSSData* sss = gm_GetGameModeStateExitData(state);

    if (!sss->start_game) {
        /* B: back to the character select. */
        gm_SetNextGameModeStateId(STATE_CSS);
        return;
    }
    training_drill_custom_set_stage(sss->vs.start.rules.stkind);
    gm_SetNextGameModeStateId(STATE_VS);
}

/* The fighters the last match loaded, once there has been one. */
static bool loaded_set;
static s8 loaded_player;
static s8 loaded_cpu;

/// Tells the loader which fighters, stage and sound banks the match needs,
/// as retail Event Match does before its matches. When the fighters are not
/// the last match's, the files the character select has been loading are
/// laid out as retail's single-player modes do (gmclassic.c): the cache
/// starts from its defaults, the player is loaded and waited for and the
/// previous match's fighters are released before the opponent and the stage
/// are added. Without that the match reads a stage file that has been
/// overwritten, and crashes. (The same fighters need none of it, and doing it
/// for them crashed the next character select.)
static void preload(int stage)
{
    struct GameCache* cache = &lbDvd_GetPreloadCacheScene()->game_cache;
    u64 mask;

    if (loaded_set &&
        (player_ckind != loaded_player || cpu_ckind != loaded_cpu))
    {
        lbDvd_80018C6C();
        cache->entries[0].char_id = player_ckind;
        cache->entries[0].color = player_color;
        lbDvd_80018254();
        lbDvd_80018C2C(0xC7);
        lbDvd_80017700(4);
    }
    loaded_set = true;
    loaded_player = player_ckind;
    loaded_cpu = cpu_ckind;
    cache->entries[0].char_id = player_ckind;
    cache->entries[0].color = player_color;
    cache->entries[1].char_id = cpu_ckind;
    cache->entries[1].color = cpu_color;
    cache->stkind = stage;
    lbDvd_80018254();
    mask = lbAudioAx_80026E84(player_ckind) |
           lbAudioAx_80026E84(cpu_ckind) | lbAudioAx_80026EBC(stage);
    lbAudioAx_80026F2C(0x1C);
    lbAudioAx_8002702C(0xC, mask);
    lbAudioAx_80027168();
}

/// The match's on_match_start (in place of the event's own setup): the drill
/// starts when the match does.
static void start_drill_match(void)
{
    LOG_EVENT_SAVE("start");
    exit_requested = false;
    training_tech_chase_init();
    training_pause_init();
}

bool training_event_enter_vs(StartMeleeData* md)
{
    int index = training_event_drill_index();
    const Drill* drill;
    int i;

    if (index < 0) {
        return false;
    }
    init_picks();
    training_drill_select(index);
    drill = training_drill_current();

    /* An endless time match (no timer), no items. The retail pause never
     * runs: Start opens our pause menu (pause.c). While it is open, retail's
     * L+R+A+Start check still runs (x3_4). */
    gm_SetupRulesDefaults(&md->rules);
    md->rules.match_kind = 0;
    md->rules.timer_enabled = 0;
    md->rules.item_freq = -1;
    md->rules.disable_pausing = 1;
    md->rules.x3_4 = 1;
    md->rules.stkind = drill->stage;
    md->rules.on_match_start = start_drill_match;

    gm_SetupAllPlayerDefaults(md->players);
    for (i = 0; i < 6; i++) {
        md->players[i].slot_type = Gm_PKind_NA;
    }
    gm_SetupHumanPlayer(&md->players[0], player_ckind, player_color, 1,
                        event_data()->x6);
    md->players[0].nametag = player_nametag;
    gm_SetupCpuPlayer(&md->players[1], cpu_ckind, cpu_color, 1, 0);
    /* No controller port, as retail's event CPUs. */
    md->players[1].slot = 0;
    md->players[1].cpu_kind = 0;
    md->players[1].cpu_level = CPU_LEVEL;
    if (md->players[1].ckind == md->players[0].ckind &&
        md->players[1].color == md->players[0].color)
    {
        md->players[1].color = (md->players[1].color + 1) %
                               gm_GetNumCostumesForCKind(md->players[1].ckind);
    }

    preload(drill->stage);
    gm_LoadRumbleEnabled(md);
    gm_80168F88();
    return true;
}

void training_event_end_match(TrainingEventExit exit)
{
    exit_requested = true;
    requested_exit = exit;
    gm_8016B328();
}

bool training_event_exit_vs(MatchExitInfo* exit)
{
    bool requested = exit_requested;

    exit_requested = false;
    if (training_event_drill_index() < 0) {
        return false;
    }
    LOG_EVENT_SAVE("exit");
    if (requested && requested_exit == TrainingEventExit_NextDrill) {
        /* The next drill in the table becomes the event in play; the match
         * reloads as a retry does, skipping the character select (the
         * fighters are the file statics above), on that drill's stage. Past
         * the last drill there is no next: back to the event list. */
        int next = training_event_drill_index() + 1;

        if (next < training_drill_count()) {
            event_data()->unk_535 = (u8) next;
            training_drill_select(next);
            gm_SetNextGameModeStateId(1);
        } else {
            gm_ChangeGameModeAfterCurrentScene(GM_MENU);
        }
        return true;
    }
    if (exit->match_end.outcome == OUTCOME_RETRY) {
        /* Reload the match scene, skipping the character select. */
        gm_SetNextGameModeStateId(1);
        return true;
    }
    if ((requested && requested_exit == TrainingEventExit_CharacterSelect) ||
        exit->match_end.outcome == OUTCOME_NO_CONTEST)
    {
        /* The character select for the same event (L+R+A+Start is a no
         * contest). The stored event is unchanged. */
        gm_SetNextGameModeStateId(0);
        return true;
    }
    /* Back to the event list, which reopens on this event. */
    gm_ChangeGameModeAfterCurrentScene(GM_MENU);
    return true;
}

#endif
