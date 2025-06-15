#ifndef __SKD_PASS_H__
#define __SKD_PASS_H__

#include <glenv.h>
#include "skd.h"
#include "camera.h"
#include "ui.h"
#include "util/shaders.h"

// contains handles to all schedule related rendering objects
typedef struct __SKD_PASS_H__SchedulePass SchedulePass;
// user-facing struct to configure SchedulePass
typedef struct {
    GLfloat color_ant[3], color_src[3];
    float globe_radius, shell_radius;
    Shader* vert;
    Shader* frag;
} SchedulePassDesc;
// initialize SchedulePass from a given Schedule
SchedulePass* SchedulePass_init_from_schedule(SchedulePassDesc desc, Schedule skd);
// free SchedulePass
void SchedulePass_free(const SchedulePass* const pass);
// update relevant uniforms and render
// provide debug == 1 for dumpable debug
// provide debug == 2 for single line
void SchedulePass_update_and_draw(SchedulePass* const pass, Schedule skd, const Camera* const cam);
// called by SchedulePass_handle_input
void SchedulePass_handle_action(SchedulePass* const pass, Schedule skd, const OverlayAction act);
// allows pausing/unpausing and resetting
void SchedulePass_handle_input(SchedulePass* const pass, Schedule skd, const RGFW_window* const win);

#endif /* __SKD_PASS_H__ */
