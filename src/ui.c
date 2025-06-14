#include "ui.h"
#include <glenv.h>
#include <math.h>
#include "action.h"
#include "util/log_new.h"

struct __UI_H__Overlay {
    struct nk_context* ctx;
    const char* skd_path;
    int counter;
};

Overlay* Overlay_init(const OverlayDesc desc, RGFW_window* win) {
    Overlay* ui = (Overlay*) malloc(sizeof(Overlay));
    ASSERT_LOG(ui, "Failed to allocate Overlay.", NULL);
    ui->ctx = glenv_init(win);
    ui->skd_path = desc.skd_path;
    ui->counter = 0;
    return ui;
}

void Overlay_free(Overlay* ui) {
    (void) ui;
}

void format_fixed_length_double(char* buf, size_t buf_size, double val) {
    int dig = (val < 1.0) ? 1 : (int) log10(fabs(val)) + 1;
    int acc = ((int) buf_size > dig + 2) ? ((int) buf_size - dig - 2) : 0;
    snprintf(buf, buf_size, "%.*f", acc, val);
}

float nk_row_height(struct nk_context* ctx) {
    return ctx->style.font->height + ctx->style.window.padding.y;
}

#define NK_MAGIC 1.555555f

float nk_win_height(struct nk_context* ctx, const char* title, enum nk_panel_flags flags, size_t rows) {
    float height = 0.f;
    const float font_size = ctx->style.font->height;
    if(flags & NK_WINDOW_BORDER) height += ctx->style.window.border * 2.f;
    if(flags & NK_WINDOW_TITLE) {
        height += font_size + \
            ctx->style.window.header.padding.y * 2.f + \
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

Action Overlay_prepare_interface(Overlay* ui, const OverlayFrameData ui_data, const RGFW_window* const win) {
    Action act = ACTION_NONE;
    const float row_height = nk_row_height(ui->ctx); float x = 0.f, y = 0.f;
    const float width = (float) win->r.w * 0.3f;
    //
    // top banner panel
    const enum nk_panel_flags banner_flags = NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_NO_INPUT;
    const float banner_height = nk_win_height(ui->ctx, "banner", 0, 1);
    const struct nk_rect banner_bounds = nk_rect(0.f, 0.f, (float) win->r.w, banner_height);
    if(nk_begin(ui->ctx, "banner", banner_bounds, banner_flags)) {
        nk_layout_row_dynamic(ui->ctx, row_height, 1);
        nk_label(ui->ctx, ui->skd_path, NK_TEXT_ALIGN_LEFT);
    } nk_end(ui->ctx);
    //
    // info panel
    const enum nk_panel_flags info_flags = NK_WINDOW_BORDER | NK_WINDOW_TITLE | NK_WINDOW_MINIMIZABLE | NK_WINDOW_NO_SCROLLBAR;
    const float info_height = nk_win_height(ui->ctx, "info", info_flags, 2);
    const struct nk_rect info_bounds = nk_rect(0.f, x += banner_height, width, x + info_height);
    if(nk_begin(ui->ctx, "info", info_bounds, info_flags)) {
        nk_layout_row_dynamic(ui->ctx, row_height, 1);
        nk_labelf(ui->ctx, NK_TEXT_LEFT, "jd: %lf", ui_data.jd);
        nk_labelf(ui->ctx, NK_TEXT_LEFT, "gmst: %lf", ui_data.gmst);
    } nk_end(ui->ctx);
    //
    // controls panel
    const enum nk_panel_flags controls_flags = NK_WINDOW_BORDER | NK_WINDOW_TITLE | NK_WINDOW_MINIMIZABLE | NK_WINDOW_NO_SCROLLBAR;
    const float controls_height = nk_win_height(ui->ctx, "controls", controls_flags, 1);
    const struct nk_rect controls_bounds = nk_rect(0.f, x += info_height, width, x + controls_height);
    if(nk_begin(ui->ctx, "controls", controls_bounds, controls_flags)) {
        nk_layout_row_dynamic(ui->ctx, row_height, 3);
        if(nk_button_label(ui->ctx, "-")) act = ACTION_SKD_PASS_SLOWER;
        nk_labelf(ui->ctx, NK_TEXT_ALIGN_CENTERED, "%llux", ui_data.speed);
        if(nk_button_label(ui->ctx, "+")) act = ACTION_SKD_PASS_FASTER;
        nk_layout_row_dynamic(ui->ctx, row_height, 2);
        if(nk_button_label(ui->ctx, ui_data.paused ? "Play" : "Pause")) act = ACTION_SKD_PASS_PAUSE;
        if(nk_button_label(ui->ctx, "Reset")) act = ACTION_SKD_PASS_RESET;
    } nk_end(ui->ctx);
    return act;
}
