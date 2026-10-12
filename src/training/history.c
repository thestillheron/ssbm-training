#ifdef TRAINING_BUILD

#include "history.h"

#include <melee/gm/gmmain_lib.h>
#include <melee/lb/inlines.h>
#include <melee/mn/inlines.h>
#include <melee/mn/mnmain.h>
#include <dolphin/gx.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/hsd_3915.h>
#include <sysdolphin/baselib/sislib.h>

#include <stdio.h>
#include <string.h>

#include "drill.h"
#include "event_list.h"
#include "score.h"
#include "text.h"

#ifdef AUTHORING_BUILD
#include <dolphin/os.h>
#endif

/* The RAM store: one entry per drill ID seen, with the latest scores (oldest
 * first) and the total attempts. Room for more drills than the table has. */
#define HISTORY_DRILLS 16
/* A score is stored in one byte. */
#define SCORE_MAX 255

typedef struct HistoryEntry {
    DrillId id; /* DrillId_None: unused */
    int total;
    int count;
    u8 scores[HISTORY_KEEP];
} HistoryEntry;

static HistoryEntry entries[HISTORY_DRILLS];

/* The save: the entries live in the training build's own save (ADR-0006),
 * in the name tag slots past the 120 a player can use (GM_NAMETAG_COUNT up
 * to the end of the last bank), which Melee never reads or writes and the
 * card file already has room for. Layout: "DHST", a version byte, the record
 * count and two zero bytes, then records of the drill id, the count of kept
 * scores, two zero bytes, the total attempts (u32, big endian) and
 * HISTORY_KEEP score bytes. A save without the magic (a new one) holds no
 * history. (The card file itself is encrypted by Melee's card library, so
 * the bytes are not readable off the card.) */
#define SAVE_VERSION 1
#define SAVE_MAGIC "DHST"
#define SAVE_MAGIC_SIZE 4
#define SAVE_VERSION_OFFSET 4
#define SAVE_COUNT_OFFSET 5
#define SAVE_HEADER_SIZE 8
/* Offsets inside a record. */
#define RECORD_ID_OFFSET 0
#define RECORD_KEPT_OFFSET 1
#define RECORD_TOTAL_OFFSET 4
#define SAVE_RECORD_HEADER_SIZE 8
#define SAVE_RECORD_SIZE (SAVE_RECORD_HEADER_SIZE + HISTORY_KEEP)
#define SAVE_SIZE (SAVE_HEADER_SIZE + HISTORY_DRILLS * SAVE_RECORD_SIZE)
#define SAVE_SPARE_BYTES                                                      \
    ((GM_NAMETAG_BANK_COUNT * GM_NAMETAG_BANK_SIZE - GM_NAMETAG_COUNT) *      \
     (int) sizeof(struct NameTagData))

typedef char history_fits_in_the_spare_name_tag_slots
    [SAVE_SIZE <= SAVE_SPARE_BYTES ? 1 : -1];

static bool loaded;

static u8* save_area(void)
{
    return (u8*) GetPersistentNameData(GM_NAMETAG_COUNT);
}

/// The `index`th record of the save area.
static u8* save_record(u8* area, int index)
{
    return area + SAVE_HEADER_SIZE + index * SAVE_RECORD_SIZE;
}

static u32 read_u32(const u8* p)
{
    return ((u32) p[0] << 24) | ((u32) p[1] << 16) | ((u32) p[2] << 8) | p[3];
}

static void write_u32(u8* p, u32 value)
{
    p[0] = (u8) (value >> 24);
    p[1] = (u8) (value >> 16);
    p[2] = (u8) (value >> 8);
    p[3] = (u8) value;
}

/// Whether the save area holds a history this version can read: the magic,
/// the version, and sane records. A save that fails any check is treated as
/// holding none.
static bool save_is_valid(const u8* area)
{
    int n;
    int i;

    if (memcmp(area, SAVE_MAGIC, SAVE_MAGIC_SIZE) != 0 ||
        area[SAVE_VERSION_OFFSET] != SAVE_VERSION ||
        area[SAVE_COUNT_OFFSET] > HISTORY_DRILLS)
    {
        return false;
    }
    n = area[SAVE_COUNT_OFFSET];
    for (i = 0; i < n; i++) {
        const u8* record = save_record((u8*) area, i);
        u8 kept = record[RECORD_KEPT_OFFSET];
        u32 total = read_u32(record + RECORD_TOTAL_OFFSET);

        if (record[RECORD_ID_OFFSET] == DrillId_None || kept > HISTORY_KEEP ||
            total < kept || total > 0x7FFFFFFF)
        {
            return false;
        }
    }
    return true;
}

/// Reads the history from the save into the RAM store, once: the first time
/// anything asks for it, which is after the card's save has been loaded.
static void load_from_save(void)
{
    u8* area;
    int n;
    int i;

    if (loaded) {
        return;
    }
    loaded = true;
    area = save_area();
    if (!save_is_valid(area)) {
        return;
    }
    n = area[SAVE_COUNT_OFFSET];
    for (i = 0; i < n; i++) {
        const u8* record = save_record(area, i);

        entries[i].id = (DrillId) record[RECORD_ID_OFFSET];
        entries[i].count = record[RECORD_KEPT_OFFSET];
        entries[i].total = (int) read_u32(record + RECORD_TOTAL_OFFSET);
        memcpy(entries[i].scores, record + SAVE_RECORD_HEADER_SIZE,
               HISTORY_KEEP);
    }
#ifdef AUTHORING_BUILD
    OSReport("[rep] event=history_load entries=%d\n", n);
#endif
}

/// Writes the RAM store into the save area and marks the save changed for
/// Melee's background write. With no card, or a declined save, the write never
/// happens and the history stays in RAM; a failed write is Melee's to note and
/// the drill summary never sees it.
static void save_to_card(void)
{
    u8* area = save_area();
    int n = 0;
    int i;

    for (i = 0; i < HISTORY_DRILLS; i++) {
        u8* record;

        if (entries[i].id == DrillId_None) {
            continue;
        }
        record = save_record(area, n);
        memset(record, 0, SAVE_RECORD_SIZE);
        record[RECORD_ID_OFFSET] = (u8) entries[i].id;
        record[RECORD_KEPT_OFFSET] = (u8) entries[i].count;
        write_u32(record + RECORD_TOTAL_OFFSET, (u32) entries[i].total);
        memcpy(record + SAVE_RECORD_HEADER_SIZE, entries[i].scores,
               entries[i].count);
        n++;
    }
    memset(area, 0, SAVE_HEADER_SIZE);
    memcpy(area, SAVE_MAGIC, SAVE_MAGIC_SIZE);
    area[SAVE_VERSION_OFFSET] = SAVE_VERSION;
    area[SAVE_COUNT_OFFSET] = (u8) n;
    /* Not plain lbCardGame_SaveChanges: it asserts that the card game library
     * is enabled, and a scene change (gm_1A3F's scene exit) resets it and
     * forgets the card work area; only lbCardGame_LoadArchive enables it
     * again, which the menu and training scenes call but a drill's match
     * scene does not. lbCardGame_SetupArchive is Melee's own AllocWorkArea +
     * LoadArchive + SaveChanges (as gmtrainingmode.c does): the first two do
     * nothing when already done and touch only the work area and icon
     * archives, never the save data, so the spare name tag bytes written
     * above are safe. */
    lbCardGame_SetupArchive();
#ifdef AUTHORING_BUILD
    OSReport("[rep] event=history_save entries=%d\n", n);
#endif
}

static HistoryEntry* find_entry(DrillId id)
{
    int i;

    load_from_save();
    for (i = 0; i < HISTORY_DRILLS; i++) {
        if (entries[i].id == id) {
            return &entries[i];
        }
    }
    return NULL;
}

/// The entry for `id`, made if it has none; NULL when the store is full.
static HistoryEntry* entry_for(DrillId id)
{
    HistoryEntry* entry = find_entry(id);

    if (entry == NULL) {
        entry = find_entry(DrillId_None);
        if (entry != NULL) {
            entry->id = id;
        }
    }
    return entry;
}

/// The entry for `id`, or an empty one when it has none.
static const HistoryEntry* entry_or_empty(DrillId id)
{
    static const HistoryEntry empty = { DrillId_None };
    const HistoryEntry* entry = find_entry(id);

    return entry != NULL ? entry : &empty;
}

int training_history_total(DrillId id)
{
    return entry_or_empty(id)->total;
}

int training_history_count(DrillId id)
{
    return entry_or_empty(id)->count;
}

int training_history_score(DrillId id, int index)
{
    const HistoryEntry* entry = entry_or_empty(id);

    if (index < 0 || index >= entry->count) {
        return 0;
    }
    return entry->scores[index];
}

/// Adds `score` as an attempt at `drill` in RAM and in the save; the oldest
/// kept score drops off at HISTORY_KEEP, the total keeps counting. Returns
/// false when the store has no room for another drill.
static bool append_score(const Drill* drill, int score)
{
    HistoryEntry* entry = entry_for(drill->id);

    if (entry == NULL) {
        return false;
    }
    if (score > SCORE_MAX) {
        score = SCORE_MAX;
    }
    if (entry->count == HISTORY_KEEP) {
        memmove(entry->scores, entry->scores + 1, HISTORY_KEEP - 1);
        entry->count--;
    }
    entry->scores[entry->count++] = (u8) score;
    entry->total++;
    save_to_card();
#ifdef AUTHORING_BUILD
    OSReport("[rep] event=history_record drill_id=%d name=\"%s\" score=%d "
             "attempts=%d\n",
             drill->id, drill->name, score, entry->total);
#endif
    return true;
}

bool training_history_record(void)
{
    const Drill* drill = training_drill_current();

    if (!drill->history || training_drill_layout_edited()) {
        return false;
    }
    return append_score(drill, training_score_successes());
}

#ifdef AUTHORING_BUILD
/// Authoring only (Y on a drill in the event list): adds SEED_ATTEMPTS
/// attempts with made-up scores, so a scenario can fill a history past
/// HISTORY_KEEP without playing a hundred attempts.
#define SEED_ATTEMPTS 30
static void seed_attempts(const Drill* drill)
{
    int range = drill->length_kind == DrillLength_Reps ? drill->length : 20;
    int i;

    for (i = 0; i < SEED_ATTEMPTS; i++) {
        append_score(drill, (i * 7) % (range + 1));
    }
}
#endif

/* The graph screen: a full-screen panel over the event list, drawn on a
 * canvas of its own (canvas units: 640 wide). */
#define SCREEN_W 640.0F
#define SCREEN_H 480.0F
#define PLOT_LEFT 110.0F
#define PLOT_RIGHT 590.0F
#define PLOT_TOP 120.0F
#define PLOT_BOTTOM 360.0F
#define AXIS_THICKNESS 3.0F
#define PASS_LINE_THICKNESS 3.0F
/* A score is a filled circle of this radius, joined to the next by a line this
 * thick. The circle is a fan of CIRCLE_SEGMENTS triangles. */
#define DOT_RADIUS 5.0F
#define LINE_THICKNESS 2.5F
#define CIRCLE_SEGMENTS 12
/* The graph shows at most this many attempts at a time, and left or right
 * moves the window by this many. */
#define GRAPH_WINDOW 30
#define GRAPH_STEP 15
/* Dots stay this far inside the plot's left and right edges. */
#define DOT_MARGIN 14.0F
#define TITLE_X 60.0F
#define TITLE_Y 40.0F
#define TITLE_SCALE 0.8F
/* "No attempts yet": its scale, and how far above the plot's middle its top
 * sits. */
#define NOTICE_SCALE 0.8F
#define NOTICE_RAISE 12.0F
#define LABEL_SCALE 0.55F
/* A label character is about 11 canvas units wide at LABEL_SCALE, and the
 * glyphs about 14 tall. */
#define LABEL_CHAR_WIDTH 11.0F
#define LABEL_HEIGHT 14.0F
#define LABEL_GAP 8.0F
#define X_LABEL_Y 374.0F
#define AXIS_TITLE_Y 402.0F
#define HINT_Y 430.0F
/* A timed drill's top is its best score rounded up to a multiple of this. */
#define TIMED_TOP_STEP 5
/* The most scores written to the rep log in one line. */
#define LOG_SCORES 24

/* Rectangles and text the graph is made of: the background, two axes, the
 * pass mark line, a dot per kept score, the labels. */
#define GRAPH_OBJECTS 40

static const GXColor background_colour = { 10, 12, 30, 255 };
static const GXColor axis_colour = { 200, 200, 200, 255 };
static const GXColor dot_colour = { 110, 205, 255, 255 };
static const GXColor pass_colour = { 255, 165, 40, 255 };
static const GXColor line_colour = { 60, 130, 190, 255 };

static HSD_Text* graph_objects[GRAPH_OBJECTS];
static int graph_object_count;
static bool graph_open;

static int graph_canvas;
/* The drill the graph shows, and the window: the attempts from
 * graph_end - GRAPH_WINDOW (or 0) up to, not including, graph_end. */
static const Drill* graph_drill;
static int graph_end;

static void track_text(HSD_Text* text)
{
    if (graph_object_count < GRAPH_OBJECTS) {
        graph_objects[graph_object_count++] = text;
    } else {
        training_text_free(&text);
    }
}

static void add_rect(float x, float y, float w, float h, GXColor colour)
{
    track_text(training_text_rect_on(graph_canvas, x, y, w, h, colour));
}

/// Adds a label with its left edge at `x` and its top at `y`.
static HSD_Text* add_label(float x, float y, float scale, const char* ascii,
                           GXColor colour)
{
    HSD_Text* text = training_text_create_on(graph_canvas);

    training_text_add(text, x, y, scale, scale, ascii);
    training_text_set_colour(text, 0, colour);
    track_text(text);
    return text;
}

static float text_width(const char* ascii, float scale)
{
    return (float) strlen(ascii) * LABEL_CHAR_WIDTH * (scale / LABEL_SCALE);
}

/// The y axis' top: a rep-count drill's length; a timed drill's best score so
/// far rounded up, and at least the pass mark so its line stays on the graph.
static int y_top(const Drill* drill, int count)
{
    int top;
    int best = 0;
    int i;

    if (drill->length_kind == DrillLength_Reps) {
        top = drill->length;
    } else {
        for (i = 0; i < count; i++) {
            if (training_history_score(drill->id, i) > best) {
                best = training_history_score(drill->id, i);
            }
        }
        top = (best + TIMED_TOP_STEP - 1) / TIMED_TOP_STEP * TIMED_TOP_STEP;
        if (top < drill->pass_mark) {
            top = drill->pass_mark;
        }
    }
    return top > 0 ? top : TIMED_TOP_STEP;
}

static float y_of(int value, int top)
{
    return PLOT_BOTTOM - (PLOT_BOTTOM - PLOT_TOP) * (float) value / (float) top;
}

/// A y-axis label for `value`, right-aligned against the axis, centred on its
/// height.
static void add_y_label(int value, int top, GXColor colour)
{
    char label[8];

    sprintf(label, "%d", value);
    add_label(PLOT_LEFT - LABEL_GAP - text_width(label, LABEL_SCALE) -
                  AXIS_THICKNESS,
              y_of(value, top) - LABEL_HEIGHT * 0.5F, LABEL_SCALE, label,
              colour);
}

static float dot_x(int index, int count)
{
    float left = PLOT_LEFT + AXIS_THICKNESS + DOT_MARGIN;
    float right = PLOT_RIGHT - DOT_MARGIN;

    if (count <= 1) {
        return (left + right) * 0.5F;
    }
    return left + (right - left) * (float) index / (float) (count - 1);
}

/* The scores' line and dots are drawn by a canvas object of their own
 * (text.h), from the points kept here in canvas units. */
typedef struct PlotPoint {
    float x;
    float y;
} PlotPoint;

static PlotPoint plot_points[HISTORY_KEEP];
static int plot_count;
static HSD_GObj* plot_gobj;

/* cos and sin of the circle's CIRCLE_SEGMENTS + 1 rim points, 30 degrees
 * apart. */
static const float circle_cos[CIRCLE_SEGMENTS + 1] = {
    1.0F, 0.866025F, 0.5F, 0.0F, -0.5F, -0.866025F, -1.0F,
    -0.866025F, -0.5F, 0.0F, 0.5F, 0.866025F, 1.0F,
};
static const float circle_sin[CIRCLE_SEGMENTS + 1] = {
    0.0F, 0.5F, 0.866025F, 1.0F, 0.866025F, 0.5F, 0.0F,
    -0.5F, -0.866025F, -1.0F, -0.866025F, -0.5F, 0.0F,
};

static void vertex(float x, float y, GXColor colour)
{
    GXPosition2f32(x, -y);
    GXColor4u8(colour.r, colour.g, colour.b, colour.a);
}

static float sqrt_f(float v)
{
    float x = v > 1.0F ? v * 0.5F : 1.0F;
    int i;

    for (i = 0; i < 8; i++) {
        x = 0.5F * (x + v / x);
    }
    return x;
}

/// A line of LINE_THICKNESS from point `a` to point `b`, as a quad.
static void draw_segment(PlotPoint a, PlotPoint b)
{
    float dx = b.x - a.x;
    float dy = b.y - a.y;
    float length = sqrt_f(dx * dx + dy * dy);
    float nx;
    float ny;

    if (length <= 0.0F) {
        return;
    }
    nx = -dy / length * LINE_THICKNESS * 0.5F;
    ny = dx / length * LINE_THICKNESS * 0.5F;
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    vertex(a.x + nx, a.y + ny, line_colour);
    vertex(b.x + nx, b.y + ny, line_colour);
    vertex(b.x - nx, b.y - ny, line_colour);
    vertex(a.x - nx, a.y - ny, line_colour);
}

static void draw_dot(PlotPoint c)
{
    int i;

    GXBegin(GX_TRIANGLEFAN, GX_VTXFMT0, CIRCLE_SEGMENTS + 2);
    vertex(c.x, c.y, dot_colour);
    for (i = 0; i <= CIRCLE_SEGMENTS; i++) {
        vertex(c.x + circle_cos[i] * DOT_RADIUS,
               c.y + circle_sin[i] * DOT_RADIUS, dot_colour);
    }
}

/// Draws the line joining the points in attempt order, then a dot on each.
static void draw_plot(HSD_GObj* gobj, int code)
{
    int i;

    hsd_80391A04(1.0F, 1.0F, 1);
    for (i = 1; i < plot_count; i++) {
        draw_segment(plot_points[i - 1], plot_points[i]);
    }
    for (i = 0; i < plot_count; i++) {
        draw_dot(plot_points[i]);
    }
}

/// An x-axis label for attempt number `number`, centred under the dot at
/// `x`.
static void add_x_label(int number, float x)
{
    char label[12];

    sprintf(label, "%d", number);
    add_label(x - text_width(label, LABEL_SCALE) * 0.5F, X_LABEL_Y,
              LABEL_SCALE, label, axis_colour);
}

#ifdef AUTHORING_BUILD
/// Writes the graph's contents to the rep log: the drill, its totals and the
/// latest scores.
static void log_graph(const Drill* drill, int total, int count, int top)
{
    int first = count > LOG_SCORES ? count - LOG_SCORES : 0;
    int i;

    OSReport("[rep] event=history_graph drill_id=%d name=\"%s\" attempts=%d "
             "kept=%d top=%d pass=%d scores=",
             drill->id, drill->name, total, count, top, drill->pass_mark);
    for (i = first; i < count; i++) {
        OSReport(i == first ? "%d" : ",%d",
                 training_history_score(drill->id, i));
    }
    OSReport("\n");
}
#define LOG_GRAPH(drill, total, count, top) \
    log_graph(drill, total, count, top)
#else
#define LOG_GRAPH(drill, total, count, top) ((void) 0)
#endif

/// Frees everything build_graph made, leaving the canvas.
static void clear_graph(void)
{
    int i;

    for (i = 0; i < graph_object_count; i++) {
        training_text_free(&graph_objects[i]);
    }
    graph_object_count = 0;
    if (plot_gobj != NULL) {
        HSD_GObjFree(plot_gobj);
        plot_gobj = NULL;
    }
}

/// Draws the graph of the window ending at graph_end on graph_canvas.
static void build_graph(void)
{
    const Drill* drill = graph_drill;
    char title[TRAINING_TEXT_MAX_CHARS + 1];
    int total = training_history_total(drill->id);
    int count = training_history_count(drill->id);
    int top = y_top(drill, count);
    int first = graph_end > GRAPH_WINDOW ? graph_end - GRAPH_WINDOW : 0;
    int shown = graph_end - first;
    int i;

    add_rect(0.0F, 0.0F, SCREEN_W, SCREEN_H, background_colour);

    sprintf(title, "%s   %d attempt%s", drill->name, total,
            total == 1 ? "" : "s");
    add_label(TITLE_X, TITLE_Y, TITLE_SCALE, title, axis_colour);
    add_label(TITLE_X, HINT_Y, LABEL_SCALE,
              count > GRAPH_WINDOW ? "Left/Right: Older/Newer     B or X: Back"
                                   : "B or X: Back",
              axis_colour);
    if (count == 0) {
        add_label((SCREEN_W - text_width("No attempts yet", NOTICE_SCALE)) *
                      0.5F,
                  (PLOT_TOP + PLOT_BOTTOM) * 0.5F - NOTICE_RAISE,
                  NOTICE_SCALE, "No attempts yet", axis_colour);
        return;
    }

    /* Axes: the y axis at the plot's left, the x axis along its bottom. */
    add_rect(PLOT_LEFT, PLOT_TOP, AXIS_THICKNESS, PLOT_BOTTOM - PLOT_TOP,
             axis_colour);
    add_rect(PLOT_LEFT, PLOT_BOTTOM, PLOT_RIGHT - PLOT_LEFT, AXIS_THICKNESS,
             axis_colour);

    /* The pass mark's line under the dots, then the labels at 0, the pass
     * mark and the top. */
    if (training_drill_has_pass_mark(drill)) {
        add_rect(PLOT_LEFT, y_of(drill->pass_mark, top) -
                                PASS_LINE_THICKNESS * 0.5F,
                 PLOT_RIGHT - PLOT_LEFT, PASS_LINE_THICKNESS, pass_colour);
    }
    add_y_label(0, top, axis_colour);
    if (training_drill_has_pass_mark(drill)) {
        add_y_label(drill->pass_mark, top, pass_colour);
    }
    if (!training_drill_has_pass_mark(drill) || drill->pass_mark != top) {
        add_y_label(top, top, axis_colour);
    }

    /* The line and the dots. */
    plot_count = shown;
    for (i = 0; i < shown; i++) {
        plot_points[i].x = dot_x(i, shown);
        plot_points[i].y =
            y_of(training_history_score(drill->id, first + i), top);
    }
    plot_gobj = training_text_canvas_add_draw(draw_plot);

    /* The x axis' first and last attempt numbers (real numbers: the oldest
     * scores drop off after HISTORY_KEEP). */
    add_x_label(total - count + first + 1, dot_x(0, shown));
    if (shown > 1) {
        add_x_label(total - count + graph_end, dot_x(shown - 1, shown));
    }
    add_label((SCREEN_W - text_width("Attempt", LABEL_SCALE)) * 0.5F,
              AXIS_TITLE_Y, LABEL_SCALE, "Attempt", axis_colour);
}

/// Opens the graph on the latest attempts.
static void open_graph(MnEventData* data, const Drill* drill)
{
    graph_open = true;
    graph_object_count = 0;
    plot_gobj = NULL;
    graph_drill = drill;
    graph_end = training_history_count(drill->id);
    training_event_list_set_visible(data, false);
    graph_canvas = training_text_canvas_create();
    LOG_GRAPH(drill, training_history_total(drill->id),
              training_history_count(drill->id),
              y_top(drill, training_history_count(drill->id)));
    build_graph();
}

/// Moves the window by `step` attempts (negative: older) and repaints; does
/// nothing at either end.
static void move_window(int step)
{
    int count = training_history_count(graph_drill->id);
    int least = count < GRAPH_WINDOW ? count : GRAPH_WINDOW;
    int end = graph_end + step;

    if (end < least) {
        end = least;
    }
    if (end > count) {
        end = count;
    }
    if (end != graph_end) {
        graph_end = end;
        clear_graph();
        build_graph();
    }
}

static void close_graph(MnEventData* data)
{
    clear_graph();
    graph_open = false;
    training_event_list_set_visible(data, true);
}

/// The drill under the event list's cursor.
static const Drill* selected_drill(const MnEventData* data)
{
    return training_drill_at(data->first_event + data->page);
}

bool training_history_input(HSD_GObj* gobj, u64* inputs)
{
    MnEventData* data;
    const Drill* drill;

    if (gobj == NULL) {
        return false;
    }
    data = gobj->user_data;

    if (graph_open) {
        if (*inputs & (MenuInput_Back | MenuInput_XButton)) {
            sfxBack();
            close_graph(data);
        } else if (*inputs & MenuInput_Left) {
            move_window(-GRAPH_STEP);
        } else if (*inputs & MenuInput_Right) {
            move_window(GRAPH_STEP);
        }
        return true;
    }
#ifdef AUTHORING_BUILD
    if (*inputs & MenuInput_YButton) {
        drill = selected_drill(data);
        if (drill->history) {
            seed_attempts(drill);
        }
        return true;
    }
#endif
    if (!(*inputs & MenuInput_XButton)) {
        return false;
    }
    drill = selected_drill(data);
    if (drill->history) {
        sfxForward();
        open_graph(data, drill);
    }
    return true;
}

#endif
