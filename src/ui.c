#include "ui.h"
#include <glenv.h>
#include <math.h>
#include "action.h"
#include "util/log_new.h"

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

// I want to avoid allocations for OverlayState,
// so I've initialized it with enough space for every
// station to scan a different source at the same time.
// This will NEVER happen in practice, so it's "safe"
#define MAX_STATION_COUNT 57

static struct {
    double jd, gmst;
    unsigned long long speed;
    unsigned int paused;
    size_t scan_idx;
    char scans[MAX_STATION_COUNT * 8 + 1];
} OverlayState;

void OverlayState_set_jd(double jd) {
    OverlayState.jd = jd;
}

void OverlayState_set_gmst(double gmst) {
    OverlayState.gmst = gmst;
}

void OverlayState_set_speed(unsigned long long speed) {
    OverlayState.speed = speed;
}

void OverlayState_set_paused(unsigned int paused) {
    OverlayState.paused = paused;
}

void OverlayState_add_source(const char* const name) {
    size_t idx = (OverlayState.scan_idx++) * 8;
    strcpy(&(OverlayState.scans[idx]), name);
}

const char* OverlayState_pop_source() {
    if(!OverlayState.scan_idx) return NULL;
    size_t idx = (--OverlayState.scan_idx) * 8;
    return &(OverlayState.scans[idx]);
}

struct __UI_H__Overlay {
    struct nk_context* ctx;
    const char* skd_path;
    size_t max_concurrent_scans;
};

Overlay* Overlay_init(const OverlayDesc desc, RGFW_window* win) {
    Overlay* ui = (Overlay*) malloc(sizeof(Overlay));
    ASSERT_LOG(ui, "Failed to allocate Overlay.", NULL);
    ui->ctx = glenv_init(win);
    ui->skd_path = desc.skd_path;
    ui->max_concurrent_scans = 0;
    return ui;
}

void Overlay_free(Overlay* ui) {
    (void) ui;
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

Action Overlay_prepare_interface(Overlay* ui, const RGFW_window* const win) {
    Action act = ACTION_NONE;
    // a few constants related to panel sizes
    const float row_height = nk_row_height(ui->ctx); float x = 0.f, y = 0.f;
    const float width_fst = (float) win->r.w * 0.3f;
    const float width_snd = (float) win->r.w * 0.2f;
    //
    // top banner panel
    const enum nk_panel_flags banner_flags = NK_WINDOW_NO_SCROLLBAR | \
        NK_WINDOW_NO_INPUT;
    // calculate banner panel's dimensions
    const float banner_height = nk_win_height(ui->ctx, "banner", 0, 1);
    const struct nk_rect banner_bounds = nk_rect(
        x, y, (float) win->r.w, banner_height
    );
    // draw the banner
    if(nk_begin(ui->ctx, "banner", banner_bounds, banner_flags)) {
        nk_layout_row_dynamic(ui->ctx, row_height, 1);
        nk_label(ui->ctx, ui->skd_path, NK_TEXT_ALIGN_LEFT);
    } nk_end(ui->ctx);
    //
    // info panel
    const enum nk_panel_flags info_flags = NK_WINDOW_BORDER | \
        NK_WINDOW_TITLE | NK_WINDOW_MINIMIZABLE | NK_WINDOW_NO_SCROLLBAR;
    // calculate height and bounds of info panel
    const float info_height = nk_win_height(ui->ctx, "info", info_flags, 2);
    const struct nk_rect info_bounds = nk_rect(
        x, y += banner_height, width_fst, y + info_height
    );
    // draw the info panel
    if(nk_begin(ui->ctx, "info", info_bounds, info_flags)) {
        nk_layout_row_dynamic(ui->ctx, row_height, 1);
        nk_labelf(ui->ctx, NK_TEXT_LEFT, "jd: %lf", OverlayState.jd);
        nk_labelf(ui->ctx, NK_TEXT_LEFT, "gmst: %lf", OverlayState.gmst);
    } nk_end(ui->ctx);
    //
    // control panel
    const enum nk_panel_flags controls_flags = NK_WINDOW_BORDER | \
        NK_WINDOW_TITLE | NK_WINDOW_MINIMIZABLE | NK_WINDOW_NO_SCROLLBAR;
    // calculate height of control panel
    const float controls_height = nk_win_height(
        ui->ctx, "controls", controls_flags, 1
    );
    // ... and its bounds
    const struct nk_rect controls_bounds = nk_rect(
        x, y += info_height, width_fst, y + controls_height
    );
    // draw the control panel
    if(nk_begin(ui->ctx, "controls", controls_bounds, controls_flags)) {
        nk_layout_row_dynamic(ui->ctx, row_height, 3);
        if(nk_button_label(ui->ctx, "-")) 
            act = ACTION_SKD_PASS_SLOWER;
        nk_labelf(ui->ctx, NK_TEXT_ALIGN_CENTERED, "%llux", OverlayState.speed);
        if(nk_button_label(ui->ctx, "+")) 
            act = ACTION_SKD_PASS_FASTER;
        nk_layout_row_dynamic(ui->ctx, row_height, 2);
        if(nk_button_label(ui->ctx, OverlayState.paused ? "Play" : "Pause")) 
            act = ACTION_SKD_PASS_PAUSE;
        if(nk_button_label(ui->ctx, "Reset")) 
            act = ACTION_SKD_PASS_RESET;
    } nk_end(ui->ctx);
    //
    // second column
    x += width_fst;
    y = banner_height;
    //
    // active scans panel
    const enum nk_panel_flags sources_flags = NK_WINDOW_BORDER | \
        NK_WINDOW_TITLE | NK_WINDOW_MINIMIZABLE | NK_WINDOW_NO_SCROLLBAR;
    // calculate the appropriate number of rows
    size_t scans_count;
    scans_count = OverlayState.scan_idx;
    if(scans_count) --scans_count;
    scans_count = MAX(scans_count, ui->max_concurrent_scans);
    ui->max_concurrent_scans = scans_count;
    // calculate the height of the active scans panel
    const float scans_height = nk_win_height(
        ui->ctx, "sources", sources_flags, scans_count
    );
    // ... and its bounds
    const struct nk_rect sources_bounds = nk_rect(
        x, y, width_snd, y + scans_height
    );
    // draw the active scans panel
    const char* source;
    if(nk_begin(ui->ctx, "scans", sources_bounds, sources_flags)) {
        nk_layout_row_dynamic(ui->ctx, row_height, 1);
        if(OverlayState.scan_idx == 0) nk_label(ui->ctx, "No active scans", NK_TEXT_ALIGN_LEFT);
        while((source = OverlayState_pop_source())) nk_text(ui->ctx, source, 8, NK_TEXT_ALIGN_LEFT);
    } else while((source = OverlayState_pop_source())); nk_end(ui->ctx);
    // return action from control panel
    return act;
}
