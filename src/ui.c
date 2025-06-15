#include "ui.h"
#include <glenv.h>
#include <math.h>

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

// I want to avoid allocations for OverlayState,
// so I've initialized it with enough space for every
// station to scan a different source at the same time.
// This will NEVER happen in practice, so it's "safe"
#define MAX_STATION_COUNT 57

#define SIZE_NAME_SRC 8
#define SIZE_NAME_STA 2

#ifndef NO_UI
static struct {
    struct nk_context* ctx;
    const char* path;
    OverlayControls controls;
    OverlayAction act;
    float row_height;
    char active_scans[MAX_STATION_COUNT * SIZE_NAME_SRC + 1];
    char stations[MAX_STATION_COUNT * SIZE_NAME_STA + 1];
} Overlay;

void Overlay_init(const char* path, RGFW_window* const win) {
    struct nk_context* ctx = glenv_init(win);
    Overlay.ctx = ctx;
    Overlay.path = path;
    Overlay.act = ACTION_NONE;
    Overlay.row_height = ctx->style.font->height + ctx->style.window.padding.y;
    Overlay.active_scans[0] = '\0';
    Overlay.stations[0] = '\0';
}

OverlayAction Overlay_get_action() {
    OverlayAction act = Overlay.act;
    Overlay.act = ACTION_NONE;
    return act;
}

void Overlay_set_controls(const OverlayControls controls) {
    Overlay.controls = controls;
}

void Overlay_add_active_scan(const char* const name) {
    size_t len = strlen(Overlay.active_scans);
    memcpy(&(Overlay.active_scans[len]), name, SIZE_NAME_SRC + 1);
}

const char* Overlay_pop_active_scan() {
    static char curr[SIZE_NAME_SRC + 1] = "";
    size_t len = strlen(Overlay.active_scans);
    if(len == 0) return NULL;
    memcpy(curr, &(Overlay.active_scans[len - SIZE_NAME_SRC]), SIZE_NAME_SRC);
    Overlay.active_scans[len - SIZE_NAME_SRC] = '\0';
    return curr;
}

void Overlay_add_station(const char* const name) {
    size_t len = strlen(Overlay.stations);
    memcpy(&(Overlay.stations[len]), name, SIZE_NAME_STA + 1);
}

const char* Overlay_pop_station() {
    static char curr[SIZE_NAME_STA + 1] = "";
    size_t len = strlen(Overlay.stations);
    if(len == 0) return NULL;
    memcpy(curr, &(Overlay.stations[len - SIZE_NAME_STA]), SIZE_NAME_STA);
    Overlay.stations[len - SIZE_NAME_STA] = '\0';
    return curr;
}

typedef enum {
    PANEL_LEFT,
    PANEL_LEFT_RATIO,
    PANEL_RIGHT,
    PANEL_RIGHT_RATIO,
} PanelDesc;

typedef struct __UI_H__Panel Panel;
typedef struct {
    nk_bool right;
    nk_bool width_prop;
    union {
        float full;
        float ratio;
    } width;
    size_t rows;
} PanelBounds;

#define PANEL_BOUNDS_LEFT(_width, _rows) {\
    .right = nk_false,\
    .width_prop = nk_false,\
    .width = { .full = (_width) },\
    .rows = (_rows)\
}

#define PANEL_BOUNDS_LEFT_RATIO(_width_ratio, _rows) {\
    .right = nk_false,\
    .width_prop = nk_true,\
    .width = { .ratio = (_width_ratio) },\
    .rows = (_rows)\
}

#define PANEL_BOUNDS_RIGHT(_width, _rows) {\
    .right = nk_true,\
    .width_prop = nk_false,\
    .width = { .full = _width },\
    .rows = (_rows)\
}

#define PANEL_BOUNDS_RIGHT_RATIO(_width_ratio, _rows) {\
    .right = nk_true,\
    .width_prop = nk_true,\
    .width = { .ratio = (_width_ratio) },\
    .rows = (_rows)\
}

struct __UI_H__Panel {
    const char* title,* parent;
    PanelBounds bounds;
    enum nk_panel_flags flags;
    void (*prepare_widgets)(const nk_bool collapsed);
};

#define NK_MAGIC 1.555555f
float Panel_win_height(Panel panel) {
    const struct nk_style style = Overlay.ctx->style;
    float height = 0.f;
    const float font_size = style.font->height;
    if(panel.flags & NK_WINDOW_BORDER) height += style.window.border * 2.f;
    if(panel.flags & NK_WINDOW_TITLE) {
        height += font_size + \
            style.window.header.padding.y * 1.f + \
            style.window.header.label_padding.y * 2.f + \
            style.window.header.spacing.y;
    }
    if(!nk_window_is_collapsed(Overlay.ctx, panel.title)) {
        const float row_height_full = Overlay.row_height + \
            style.window.padding.y + \
            style.window.spacing.y;
        height += row_height_full * (float) panel.bounds.rows;
    }
    return height + NK_MAGIC;
}

//
// BEGINNING OF PANEL DEFINITIONS
//

void prepare_widgets_banner(const nk_bool collapsed) {
    if(collapsed) return;
    nk_layout_row_dynamic(Overlay.ctx, Overlay.row_height, 1);
    nk_label(Overlay.ctx, Overlay.path, NK_TEXT_ALIGN_LEFT);
}

void prepare_widgets_info(const nk_bool collapsed) {
    if(collapsed) return;
    nk_layout_row_dynamic(Overlay.ctx, Overlay.row_height, 1);
    nk_labelf(Overlay.ctx, NK_TEXT_LEFT, "jd: %lf", Overlay.controls.jd);
    nk_labelf(Overlay.ctx, NK_TEXT_LEFT, "gmst: %lf", Overlay.controls.gmst);
}

void prepare_widgets_controls(const nk_bool collapsed) {
    if(collapsed) return;
    nk_layout_row_dynamic(Overlay.ctx, Overlay.row_height, 3);
    if(nk_button_label(Overlay.ctx, "-")) 
        Overlay.act = ACTION_SKD_PASS_SLOWER;
    nk_labelf(Overlay.ctx, NK_TEXT_ALIGN_CENTERED, "%llux", Overlay.controls.speed);
    if(nk_button_label(Overlay.ctx, "+")) 
        Overlay.act = ACTION_SKD_PASS_FASTER;
    nk_layout_row_dynamic(Overlay.ctx, Overlay.row_height, 2);
    if(nk_button_label(Overlay.ctx, Overlay.controls.paused ? "Play" : "Pause")) 
        Overlay.act = ACTION_SKD_PASS_PAUSE;
    if(nk_button_label(Overlay.ctx, "Reset")) 
        Overlay.act = ACTION_SKD_PASS_RESET;
}

void prepare_widgets_active_scans(const nk_bool collapsed) {
    const char* source;
    if(collapsed) {
        while((source = Overlay_pop_active_scan()));
        return;
    }
    nk_layout_row_dynamic(Overlay.ctx, Overlay.row_height, 1);
    if(!strlen(Overlay.active_scans)) nk_label(Overlay.ctx, "No active scans", NK_TEXT_ALIGN_LEFT);
    while((source = Overlay_pop_active_scan())) nk_label(Overlay.ctx, source, NK_TEXT_ALIGN_LEFT);
}

void prepare_widgets_stations(const nk_bool collapsed) {
    const char* station;
    if(collapsed) {
        while((station = Overlay_pop_station()));
        return;
    }
    nk_layout_row_dynamic(Overlay.ctx, Overlay.row_height, 1);
    if(!strlen(Overlay.stations)) nk_label(Overlay.ctx, "All stations idle", NK_TEXT_ALIGN_LEFT);
    while((station = Overlay_pop_station())) nk_label(Overlay.ctx, station, NK_TEXT_ALIGN_LEFT);
}

static Panel OverlayPanels[] = {
    {
        .title = "banner",
        .parent = "",
        .bounds = PANEL_BOUNDS_LEFT_RATIO(1.f, 1),
        .flags = NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_NO_INPUT,
        .prepare_widgets = prepare_widgets_banner,
    },
    {
        .title = "info",
        .parent = "banner",
        .bounds = PANEL_BOUNDS_LEFT_RATIO(0.3f, 2),
        .flags = NK_WINDOW_BORDER | NK_WINDOW_TITLE | NK_WINDOW_MINIMIZABLE | NK_WINDOW_NO_SCROLLBAR,
        .prepare_widgets = prepare_widgets_info,
    },
    {
        .title = "controls",
        .parent = "info",
        .bounds = PANEL_BOUNDS_LEFT_RATIO(0.3f, 2),
        .flags = NK_WINDOW_BORDER | NK_WINDOW_TITLE | NK_WINDOW_MINIMIZABLE | NK_WINDOW_NO_SCROLLBAR,
        .prepare_widgets = prepare_widgets_controls,
    },
    {
        .title = "active scans",
        .parent = "banner",
        .bounds = PANEL_BOUNDS_RIGHT_RATIO(0.2f, 5),
        .flags = NK_WINDOW_BORDER | NK_WINDOW_TITLE | NK_WINDOW_MINIMIZABLE,
        .prepare_widgets = prepare_widgets_active_scans,
    },
    {
        .title = "stations",
        .parent = "active scans",
        .bounds = PANEL_BOUNDS_RIGHT_RATIO(0.2f, 5),
        .flags = NK_WINDOW_BORDER | NK_WINDOW_TITLE | NK_WINDOW_MINIMIZABLE,
        .prepare_widgets = prepare_widgets_stations,
    }
};

//
// END OF PANEL DEFINITIONS
//

Panel* Panel_parent(Panel panel) {
    for(size_t i = 0; i < (sizeof(OverlayPanels) / sizeof(OverlayPanels[0])); ++i) {
        if(!strcmp(OverlayPanels[i].title, panel.parent)) return &(OverlayPanels[i]);
    }
    return NULL;
}

void Panel_render(Panel panel, const RGFW_window* const win) {
    float y = 0.0;
    for(Panel* curr = Panel_parent(panel); curr; curr = Panel_parent(*curr))
        y += Panel_win_height(*curr);
    const float width = panel.bounds.width_prop ? \
        panel.bounds.width.ratio * (float) win->r.w : \
        panel.bounds.width.full;
    const struct nk_rect bounds = nk_rect(
        panel.bounds.right ? (float) win->r.w - width : 0.f, y,
        width, Panel_win_height(panel)
    );
    nk_bool expanded = nk_begin(Overlay.ctx, panel.title, bounds, panel.flags);
    panel.prepare_widgets(!expanded);
    nk_end(Overlay.ctx);
}

void Overlay_prepare_interface(const RGFW_window *const win) {
    for(size_t i = 0; i < (sizeof(OverlayPanels) / sizeof(OverlayPanels[0])); ++i) 
        Panel_render(OverlayPanels[i], win);
}

#endif /* not NO_UI */
