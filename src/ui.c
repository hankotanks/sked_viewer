#include "ui.h"

#include <GL/glew.h>
#include <string.h>
#include "util/shaders.h"
#include "util/log.h"

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

#define PADDING 5
#define PANEL_WIDTH_SCALAR 0.7

inline GLsizei Panel_width(Panel sub, GLint window_width, GLsizei pt) {
    if(sub.line_char_limit == -1) return window_width;
    float full = (float) sub.line_char_limit * (float) pt;
    return (GLsizei) (full * PANEL_WIDTH_SCALAR);
}

inline GLsizei Panel_height(Panel sub, GLsizei pt) {
    return sub.max_lines * (pt + PADDING);
}

Panel Panel_top_left(const Panel* const prev, OverlayData data) {
    Panel sub = (Panel) { .prev = prev, .y = PADDING, .line_char_limit = 0 };
    Panel_finish(&sub, data);
    return sub;
}

Panel Panel_bottom_banner(OverlayData data) {
    return (Panel) {
        .prev = NULL,
        .x = 0,
        .y = (GLint) abs((int) (data.pt - data.window_size[1] + PADDING)),
        .max_lines = 1,
        .idx = 0,
        .line_char_limit = -1,
    };
}

// assumes program has been set by OverlayUI
void Panel_draw(Panel sub, OverlayData data) {
    GLsizei w, h;
    w = Panel_width(sub, data.window_size[0], data.pt);
    h = Panel_height(sub, data.pt);
    glEnable(GL_SCISSOR_TEST);
    glScissor(sub.x, data.window_size[1] - sub.y - h, w, h);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);
}

void Panel_add_line(Panel* sub, OverlayData data, const char* text) {
    float i, x, y;
    i = (float) (sub->idx)++;
    x = (float) (sub->x + PADDING);
    y = (float) (sub->y + i * (data.pt + PADDING));
    if(sub->line_char_limit == -1) {
        RFont_draw_text(data.font, text, x, y, data.pt);
    } else {
        sub->line_char_limit = MAX(sub->line_char_limit, (ssize_t) strlen(text));
        RFont_draw_text_len(data.font, text, sub->line_char_limit, x, y, data.pt, 1.f);
    }
}

void Panel_finish(Panel* sub, OverlayData data) {
    if(sub->prev) {
        GLsizei w = Panel_width(*(sub->prev), data.window_size[0], data.pt);
        sub->x = w + PADDING * 2;
    } else if(sub->line_char_limit != -1) {
        sub->x = PADDING;
    }
    sub->idx = 0;
}

unsigned int OverlayUI_init(OverlayUI* ui, OverlayDesc desc, RGFW_window* win) {
    // initialize font and populate overlay data
    OverlayData data;
    RFont_init(win->r.w, win->r.h);
    // compile font
    data.font = RFont_font_init(desc.font_path);
    if(data.font == NULL) {
        LOG_ERROR("Unable to compile front for OverlayUI.");
        return 1;
    }
    data.pt = desc.font_size;
    data.window_size[0] = (GLint) win->r.w;
    data.window_size[1] = (GLint) win->r.h;
    ui->data = data;
    // set text color
    ui->text_color[0] = desc.text_color[0];
    ui->text_color[1] = desc.text_color[1];
    ui->text_color[2] = desc.text_color[2];
    // set panel color for clearing of panel backgrounds
    ui->panel_color[0] = desc.panel_color[0];
    ui->panel_color[1] = desc.panel_color[1];
    ui->panel_color[2] = desc.panel_color[2];
    // build panel
    ui->sub[PANEL_PARAMS] = Panel_top_left(NULL, ui->data);
    ui->sub[PANEL_ACTIVE_SCANS] = Panel_top_left(&(ui->sub[PANEL_PARAMS]), ui->data);
    ui->sub[PANEL_STATUS_BAR] = Panel_bottom_banner(ui->data);
    return 0;
}

void OverlayUI_set_panel_max_lines(OverlayUI* ui, PanelID id, GLsizei max_lines) {
    ui->sub[id].max_lines = max_lines;
}

void OverlayUI_free(OverlayUI ui) {
    RFont_font_free(ui.data.font);
}

#define MIN_PT 10
#define MAX_PT 32

void OverlayUI_handle_events(OverlayUI* ui, RGFW_window* win) {
    OverlayData* data = &(ui->data);
    switch(win->event.type) {
        case RGFW_windowResized:
            RFont_update_framebuffer(win->r.w, win->r.h);
            data->window_size[0] = (GLint) win->r.w;
            data->window_size[1] = (GLint) win->r.h;
            for(size_t i = 0; i < PANEL_COUNT; ++i) {
                if(ui->sub[i].line_char_limit == -1) 
                    ui->sub[i].y = (GLint) abs((int) (data->pt - data->window_size[1] + PADDING));
            }
            break;
        case RGFW_keyPressed:
            if(RGFW_isPressed(win, RGFW_controlL) || RGFW_isPressed(win, RGFW_controlR)) switch(win->event.key) {
                case RGFW_equals:
                    data->pt = MIN(data->pt + 2, MAX_PT);
                    break;
                case RGFW_minus:
                    data->pt = MAX(data->pt - 2, MIN_PT);
                default: break;
            };
            break;
        default: break;
    }
}

void OverlayUI_draw_panel(OverlayUI* ui, PanelID id, char* const text[]) {
    OverlayData data = ui->data;
    glClearColor(ui->panel_color[0], ui->panel_color[1], ui->panel_color[2], 1.f);
    Panel_draw(ui->sub[id], data);
    RFont_set_color(ui->text_color[0], ui->text_color[1], ui->text_color[2], 1.f);
    for(size_t i = 0; text[i]; ++i) Panel_add_line(&(ui->sub[id]), data, text[i]);
    glClearColor(0.f, 0.f, 0.f, 1.f);
    Panel_finish(&(ui->sub[id]), data);
}