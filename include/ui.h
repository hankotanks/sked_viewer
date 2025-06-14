#ifndef __UI_H__
#define __UI_H__

#include <glenv.h>

typedef struct {
    const char* skd_path;
} OverlayDesc;

typedef struct {
    double jd, gmst;
} OverlayFrameData;

typedef struct __UI_H__Overlay Overlay;

Overlay* Overlay_init(const OverlayDesc desc, RGFW_window* win);
void Overlay_free(Overlay* ui);
void Overlay_prepare_interface(Overlay* ui, const OverlayFrameData ui_data, const RGFW_window* const win);

#endif /* __UI_H__ */
