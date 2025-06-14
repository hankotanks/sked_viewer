#ifndef __UI_H__
#define __UI_H__

#include <glenv.h>
#include "action.h"

typedef struct {
    const char* skd_path;
} OverlayDesc;

void OverlayState_set_jd(double jd);
void OverlayState_set_gmst(double gmst);
void OverlayState_set_speed(unsigned long long speed);
void OverlayState_set_paused(unsigned int paused);
void OverlayState_add_source(const char* const name);

typedef struct __UI_H__Overlay Overlay;

Overlay* Overlay_init(const OverlayDesc desc, RGFW_window* win);
void Overlay_free(Overlay* ui);
Action Overlay_prepare_interface(Overlay* ui, const RGFW_window* const win);

#endif /* __UI_H__ */
