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

#ifndef NO_UI
static struct {
    struct nk_context* ctx;
    const char* path;
    OverlayControls controls;
    OverlayAction act;
    size_t active_scans_curr, active_scans_max;
    char active_scans[MAX_STATION_COUNT * 8 + 1];
} Overlay;

void Overlay_init(const char* path, RGFW_window* const win) {
    Overlay.ctx = glenv_init(win);
    Overlay.path = path;
    Overlay.act = ACTION_NONE;
    Overlay.active_scans_curr = 0;
    Overlay.active_scans_max = 0;
}

OverlayAction Overlay_get_action() {
    OverlayAction act = Overlay.act;
    Overlay.act = ACTION_NONE;
    return act;
}

void Overlay_set_controls(const OverlayControls controls) {
    Overlay.controls = controls;
}

void OverlayState_add_source(const char* const name) {
    size_t idx = (Overlay.active_scans_curr++) * 8;
    strcpy(&(Overlay.active_scans[idx]), name);
}

const char* OverlayState_pop_source() {
    if(!Overlay.active_scans_curr) return NULL;
    size_t idx = (--Overlay.active_scans_curr) * 8;
    return &(Overlay.active_scans[idx]);
}

float nk_row_height(struct nk_context* ctx) {
    return ctx->style.font->height + ctx->style.window.padding.y;
}

#define NK_MAGIC 1.555555f

float nk_win_height(
    struct nk_context* ctx, 
    const char* title, 
    enum nk_panel_flags flags, 
    size_t rows
) {
    float height = 0.f;
    const float font_size = ctx->style.font->height;
    if(flags & NK_WINDOW_BORDER) height += ctx->style.window.border * 2.f;
    if(flags & NK_WINDOW_TITLE) {
        height += font_size + \
            ctx->style.window.header.padding.y * 1.f + \
            ctx->style.window.header.label_padding.y * 2.f + \
            ctx->style.window.header.spacing.y;
    }
    if(!nk_window_is_collapsed(ctx, title)) {
        const float row_height_full = nk_row_height(ctx) + \
            ctx->style.window.padding.y + \
            ctx->style.window.spacing.y;
        height += row_height_full * (float) rows + NK_MAGIC;
    }
    return height;
}

void Overlay_prepare_interface(const RGFW_window* const win) {
    // a few constants related to panel sizes
    const float row_height = nk_row_height(Overlay.ctx); float x = 0.f, y = 0.f;
    const float width_fst = (float) win->r.w * 0.3f;
    const float width_snd = (float) win->r.w * 0.2f;
    //
    // top banner panel
    const enum nk_panel_flags banner_flags = NK_WINDOW_NO_SCROLLBAR | \
        NK_WINDOW_NO_INPUT;
    // calculate banner panel's dimensions
    const float banner_height = nk_win_height(Overlay.ctx, "banner", 0, 1);
    const struct nk_rect banner_bounds = nk_rect(
        x, y, (float) win->r.w, banner_height
    );
    // draw the banner
    if(nk_begin(Overlay.ctx, "banner", banner_bounds, banner_flags)) {
        nk_layout_row_dynamic(Overlay.ctx, row_height, 1);
        nk_label(Overlay.ctx, Overlay.path, NK_TEXT_ALIGN_LEFT);
    } nk_end(Overlay.ctx);
    //
    // info panel
    const enum nk_panel_flags info_flags = NK_WINDOW_BORDER | \
        NK_WINDOW_TITLE | NK_WINDOW_MINIMIZABLE | NK_WINDOW_NO_SCROLLBAR;
    // calculate height and bounds of info panel
    const float info_height = nk_win_height(Overlay.ctx, "info", info_flags, 2);
    const struct nk_rect info_bounds = nk_rect(
        x, y += banner_height, width_fst, y + info_height
    );
    // draw the info panel
    if(nk_begin(Overlay.ctx, "info", info_bounds, info_flags)) {
        nk_layout_row_dynamic(Overlay.ctx, row_height, 1);
        nk_labelf(Overlay.ctx, NK_TEXT_LEFT, "jd: %lf", Overlay.controls.jd);
        nk_labelf(Overlay.ctx, NK_TEXT_LEFT, "gmst: %lf", Overlay.controls.gmst);
    } nk_end(Overlay.ctx);
    //
    // control panel
    const enum nk_panel_flags controls_flags = NK_WINDOW_BORDER | \
        NK_WINDOW_TITLE | NK_WINDOW_MINIMIZABLE | NK_WINDOW_NO_SCROLLBAR;
    // calculate height of control panel
    const float controls_height = nk_win_height(
        Overlay.ctx, "controls", controls_flags, 1
    );
    // ... and its bounds
    const struct nk_rect controls_bounds = nk_rect(
        x, y += info_height, width_fst, y + controls_height
    );
    // draw the control panel
    if(nk_begin(Overlay.ctx, "controls", controls_bounds, controls_flags)) {
        nk_layout_row_dynamic(Overlay.ctx, row_height, 3);
        if(nk_button_label(Overlay.ctx, "-")) 
            Overlay.act = ACTION_SKD_PASS_SLOWER;
        nk_labelf(Overlay.ctx, NK_TEXT_ALIGN_CENTERED, "%llux", Overlay.controls.speed);
        if(nk_button_label(Overlay.ctx, "+")) 
            Overlay.act = ACTION_SKD_PASS_FASTER;
        nk_layout_row_dynamic(Overlay.ctx, row_height, 2);
        if(nk_button_label(Overlay.ctx, Overlay.controls.paused ? "Play" : "Pause")) 
            Overlay.act = ACTION_SKD_PASS_PAUSE;
        if(nk_button_label(Overlay.ctx, "Reset")) 
            Overlay.act = ACTION_SKD_PASS_RESET;
    } nk_end(Overlay.ctx);
    //
    // second column
    x += width_fst;
    y = banner_height;
    //
    // active scans panel
    const enum nk_panel_flags sources_flags = NK_WINDOW_BORDER | \
        NK_WINDOW_TITLE | NK_WINDOW_MINIMIZABLE | NK_WINDOW_NO_SCROLLBAR;
    // calculate the appropriate number of rows
    size_t active_scans;
    active_scans = Overlay.active_scans_curr;
    if(active_scans) --active_scans;
    Overlay.active_scans_max = MAX(active_scans, Overlay.active_scans_max);
    // calculate the height of the active scans panel
    const float scans_height = nk_win_height(
        Overlay.ctx, "sources", sources_flags, Overlay.active_scans_max
    );
    // ... and its bounds
    const struct nk_rect sources_bounds = nk_rect(
        x, y, width_snd, y + scans_height
    );
    // draw the active scans panel
    const char* source;
    if(nk_begin(Overlay.ctx, "scans", sources_bounds, sources_flags)) {
        nk_layout_row_dynamic(Overlay.ctx, row_height, 1);
        if(Overlay.active_scans_curr == 0) nk_label(Overlay.ctx, "No active scans", NK_TEXT_ALIGN_LEFT);
        while((source = OverlayState_pop_source())) nk_text(Overlay.ctx, source, 8, NK_TEXT_ALIGN_LEFT);
    } else while((source = OverlayState_pop_source())); nk_end(Overlay.ctx);
}
#endif /* not NO_UI */
