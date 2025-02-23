#ifndef UI_H
#define UI_H

#include <GL/glew.h>
#include <stddef.h>
#include "RGFW/RGFW.h"
#include "RFont/RFont.h"
#include "flags.h"
#include "util/shaders.h"

typedef struct __Panel Panel;
struct __Panel {
    const Panel* prev;
    GLint x, y;
    GLsizei max_lines;
    size_t idx;
    ssize_t line_char_limit;
};

typedef struct {
    RFont_font* font;
    GLsizei pt;
    GLint window_size[2];
} OverlayData;

Panel Panel_top_left(const Panel* const prev, OverlayData data);
Panel Panel_bottom_banner(OverlayData data);

void Panel_draw(Panel sub, OverlayData data);
void Panel_add_line(Panel* sub, OverlayData data, const char* text);
void Panel_finish(Panel* sub, OverlayData data);

typedef enum {
    PANEL_ACTIVE_SCANS,
    PANEL_PARAMS,
    PANEL_STATUS_BAR,
    PANEL_COUNT,
} PanelID;

typedef struct {
    OverlayData data;
    float text_color[3], panel_color[3];
    Panel sub[PANEL_COUNT];
} OverlayUI;

typedef struct {
    const char* font_path;
    size_t font_size;
    float text_color[3], panel_color[3];
} OverlayDesc;

unsigned int OverlayUI_init(OverlayUI* ui, OverlayDesc desc, RGFW_window* win);

void OverlayUI_set_panel_max_lines(OverlayUI* ui, PanelID id, GLsizei max_lines);

void OverlayUI_free(OverlayUI ui);

void OverlayUI_handle_events(OverlayUI* ui, RGFW_window* win);

void OverlayUI_draw_panel(OverlayUI* ui, PanelID id, char* const text[]);

#endif // UI_H
