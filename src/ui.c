#include "ui.h"

#include <GL/glew.h>
#include <string.h>
#include "util/fio.h"
#include "util/shaders.h"
#include "util/log.h"

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

#define PADDING 5

typedef struct {
    RFont_font* font;
    size_t font_size;
    size_t window_size[2];
} OverlayData;

typedef struct __UI_H__Panel Panel;
struct __UI_H__Panel {
    const Panel* prev;
    GLint x, y;
    size_t max_lines;
    size_t idx;
    ssize_t line_char_limit;
};

inline GLsizei Panel_width(Panel sub, OverlayData data) {
    if(sub.line_char_limit == -1) {
        return (GLsizei) data.window_size[0];
    } else {
        char temp[(size_t) sub.line_char_limit + 1];
        temp[(size_t) sub.line_char_limit] = '\0';
        for(size_t i = 0; i < (size_t) sub.line_char_limit; i++) temp[i] = 'M';
        RFont_area bounds = RFont_text_area(data.font, temp, (u32) data.font_size);
        return (GLsizei) bounds.w;
    }
}

inline GLsizei Panel_height(Panel sub, OverlayData data) {
    return (GLsizei) (sub.max_lines * (data.font_size + PADDING));
}

void Panel_finish(Panel* sub, OverlayData data) {
    if(sub->prev) {
        GLsizei w = Panel_width(*(sub->prev), data);
        sub->x = w + PADDING * 2;
    } else if(sub->line_char_limit != -1) {
        sub->x = PADDING;
    } else {
        sub->y = (GLint) abs((int) (data.font_size - data.window_size[1] + PADDING));
    }
    sub->idx = 0;
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
        .y = (GLint) abs((int) (data.font_size - data.window_size[1] + PADDING)),
        .max_lines = 1,
        .idx = 0,
        .line_char_limit = -1,
    };
}

// assumes program has been set by OverlayUI
void Panel_draw(Panel sub, OverlayData data) {
    GLsizei w, h;
    w = Panel_width(sub, data);
    h = Panel_height(sub, data);
    GLint x, y;
    x = sub.x;
    y = (GLint) data.window_size[1] - sub.y - h;
    glEnable(GL_SCISSOR_TEST);
    glScissor(x, y, w, h);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);
}

void Panel_add_line(Panel* sub, OverlayData data, const char* text) {
    float i, x, y;
    i = (float) sub->idx++;
    x = (float) sub->x + PADDING;
    y = (float) sub->y + i * (float) (data.font_size + PADDING);
    if(sub->line_char_limit == -1) {
        RFont_draw_text(data.font, text, x, y, (u32) data.font_size);
    } else {
        sub->line_char_limit = MAX(sub->line_char_limit, (ssize_t) strlen(text));
        RFont_draw_text_len(data.font, text, (size_t) sub->line_char_limit, x, y, (u32) data.font_size, 1.f);
    }
}

#define FONT_SIZE_MULT 2

struct __UI_H__OverlayUI {
    const char* font_bytes;
    OverlayData data;
    float text_color[3], panel_color[3];
    Panel sub[PANEL_COUNT];
    size_t font_size_extrema[2];
};

OverlayUI* OverlayUI_init(OverlayDesc desc, RGFW_window* win) {
    // initialize font and populate overlay data
    OverlayData data;
    if(win->r.w < 0 || win->r.h < 0) {
        LOG_ERROR("Invalid window dimensions retrived from RGFW. Unable to initialize RFont.");
        return NULL;
    }
    RFont_init((size_t) win->r.w, (size_t) win->r.h);
    // initialize OverlayUI object
    OverlayUI* ui = (OverlayUI*) malloc(sizeof(OverlayUI));
    if(ui == NULL) {
        LOG_ERROR("Unable to allocate space for OverlayUI.");
        return NULL;
    }
    ui->font_bytes = read_file_contents(desc.font_path);
    // compile font
    data.font = RFont_font_init_data((b8*) ui->font_bytes, 0);
    if(data.font == NULL) {
        LOG_ERROR("Unable to compile font for OverlayUI.");
        return NULL;
    }
    // populate OverlayData
    data.font_size = desc.font_size;
    data.window_size[0] = (size_t) win->r.w;
    data.window_size[1] = (size_t) win->r.h;
    // copy data to OverlayUI
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
    // font size bounds
    ui->font_size_extrema[0] = ui->data.font_size / FONT_SIZE_MULT;
    ui->font_size_extrema[1] = ui->data.font_size * FONT_SIZE_MULT;
    return ui;
}

void OverlayUI_set_panel_max_lines(OverlayUI* const ui, PanelID id, size_t max_lines) {
    ui->sub[id].max_lines = max_lines;
}

void OverlayUI_free(OverlayUI* ui) {
    RFont_font_free(ui->data.font);
    RFont_close();
    free((char*) ui->font_bytes);
    free(ui);
}

unsigned int OverlayUI_handle_events(OverlayUI* const ui, RGFW_window* win) {
    OverlayData* data = &(ui->data);
    unsigned int update_panels = 0;
    switch(win->event.type) {
        case RGFW_windowResized:
            if(win->r.w < 0 || win->r.h < 0) {
                LOG_ERROR("Invalid window dimensions retrieved from RGFW. Unable to update RFont framebuffer.");
                return 1;
            }
            RFont_update_framebuffer((size_t) win->r.w, (size_t) win->r.h);
            data->window_size[0] = (size_t) win->r.w;
            data->window_size[1] = (size_t) win->r.h;
            update_panels = 1;
            break;
        case RGFW_keyPressed:
            if(RGFW_isPressed(win, RGFW_controlL) || RGFW_isPressed(win, RGFW_controlR)) switch(win->event.key) {
                case RGFW_equals:
                    data->font_size = MIN(data->font_size + 2, ui->font_size_extrema[1]);
                    RFont_font_free(data->font);
                    RFont_close();
                    RFont_init(data->window_size[0], data->window_size[1]);
                    data->font = RFont_font_init_data((b8*) ui->font_bytes, 0);
                    if(data->font == NULL) {
                        LOG_ERROR("Unable to recompile font for OverlayUI.");
                        return 1;
                    }
                    update_panels = 1;
                    break;
                case RGFW_minus:
                    data->font_size = MAX(data->font_size - 2, ui->font_size_extrema[0]);
                    RFont_font_free(data->font);
                    RFont_close();
                    RFont_init(data->window_size[0], data->window_size[1]);
                    data->font = RFont_font_init_data((b8*) ui->font_bytes, 0);
                    if(data->font == NULL) {
                        LOG_ERROR("Unable to recompile font for OverlayUI.");
                        return 1;
                    }
                    update_panels = 1;
                default: break;
            };
            break;
        default: break;
    }
    if(update_panels) {
        for(size_t i = 0; i < PANEL_COUNT; ++i) 
            Panel_finish(&(ui->sub[i]), *data);
    }
    return 0;
}

void OverlayUI_draw_panel(OverlayUI* const ui, PanelID id, char* const text[]) {
    // printf("x: %d, y: %d, w: %d, h: %d, win: [%zu, %zu]\n", ui->sub[id].x, ui->sub[id].y, Panel_width(ui->sub[id], ui->data.window_size[0], ui->data.font_size), Panel_height(ui->sub[id], ui->data.font_size), ui->data.window_size[0], ui->data.window_size[1]);
    OverlayData data = ui->data;
    glClearColor(ui->panel_color[0], ui->panel_color[1], ui->panel_color[2], 1.f);
    Panel_draw(ui->sub[id], data);
    RFont_set_color(ui->text_color[0], ui->text_color[1], ui->text_color[2], 1.f);
    for(size_t i = 0; text[i]; ++i) Panel_add_line(&(ui->sub[id]), data, text[i]);
    glClearColor(0.f, 0.f, 0.f, 1.f);
    Panel_finish(&(ui->sub[id]), data);
}