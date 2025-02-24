#ifndef UI_H
#define UI_H

#include <GL/glew.h>
#include <stddef.h>
#include "RGFW/RGFW.h"
#include "RFont/RFont.h"
#include "flags.h"
#include "util/shaders.h"

// allows pushing text to specific panels from outside ui.c
typedef enum {
    PANEL_ACTIVE_SCANS,
    PANEL_PARAMS,
    PANEL_STATUS_BAR,
    PANEL_COUNT,
} PanelID;
// configuration options for OverlayUI
typedef struct {
    const char* font_path;
    size_t font_size;
    float text_color[3], panel_color[3];
} OverlayDesc;
// OverlayUI is opaque to outside files
typedef struct __UI_H__OverlayUI OverlayUI;
// initialize OverlayUI
OverlayUI* OverlayUI_init(OverlayDesc desc, RGFW_window* win);
// update the height of a panel by line count
void OverlayUI_set_panel_max_lines(OverlayUI* const ui, PanelID id, size_t max_lines);
// free OverlayUI (notably the RFont_font instance within OverlayData)
void OverlayUI_free(OverlayUI* ui);
// handle resizes and font scaling
unsigned int OverlayUI_handle_events(OverlayUI* const ui, RGFW_window* win);
// push text to a panel and render it
void OverlayUI_draw_panel(OverlayUI* const ui, PanelID id, char* const text[]);

#endif // UI_H
