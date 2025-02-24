#include <GL/glew.h>
#include <stdio.h>
#include <stdint.h>
#include "RGFW/RGFW.h"
#include "RFont/RFont.h"
#include "flags.h"
#include "util/shaders.h"
#include "globe.h"
#include "camera.h"
#include "skd.h"
#include "skd_pass.h"
#ifndef DISABLE_OVERLAY_UI
#include "ui.h"
#endif

// window configuration options
#define WINDOW_TITLE "sked_viewer"
#define WINDOW_BOUNDS RGFW_RECT(0, 0, 800, 600)

// earth configuration options
#define GLOBE_TEX_OFFSET 0.f
#define GLOBE_CONFIG (GlobeConfig) {\
    .slices = 64,\
    .stacks = 48,\
    .globe_radius = 100.f,\
}

// camera configuration options
#define CAMERA_SENSITIVITY 0.002f
#define CAMERA_SCALAR 4.f
#define CAMERA_CONFIG (CameraConfig) {\
    .scalar = CAMERA_SCALAR,\
    .fov = (float) M_PI_2,\
    .z_near = 1.f,\
    .z_far = CAMERA_SCALAR * GLOBE_CONFIG.globe_radius * 2.f,\
}

// SchedulePass configuration options
#define SKD_PASS_INCREMENT 0.000075f

int main() {
    unsigned int failure;
    // build and validate Schedule
    Schedule skd;
    Schedule_build_from_source(&skd, "./assets/r41192.skd");
    failure = Schedule_debug_and_validate(skd, 0);
    if(failure) abort();
    // set up window
    RGFW_window* window = RGFW_createWindow(WINDOW_TITLE, WINDOW_BOUNDS, RGFW_windowCenter);
    glewInit();
    glClearColor(0.f, 0.f, 0.f, 1.f);
    // configure camera and set aspect
    CameraController* camera_controller = CameraController_init(CAMERA_SENSITIVITY);
    if(camera_controller == NULL) abort();
    Camera* camera = Camera_init(CAMERA_CONFIG, GLOBE_CONFIG.globe_radius);
    if(camera == NULL) abort();
    Camera_set_aspect(camera, window);
    Camera_perspective(camera, CAMERA_CONFIG);
    // set up shaders
    Shader globe_vert, sched_vert, globe_frag, sched_frag;
    globe_vert = Shader_init("./shaders/globe.vs", GL_VERTEX_SHADER);
    sched_vert = Shader_init("./shaders/sched.vs", GL_VERTEX_SHADER);
    globe_frag = Shader_init("./shaders/globe.fs", GL_FRAGMENT_SHADER);
    sched_frag = Shader_init("./shaders/sched.fs", GL_FRAGMENT_SHADER);
    // build Globe mesh
    const Globe* const globe_mesh = Globe_generate(GLOBE_CONFIG);
    if(globe_mesh == NULL) abort();
    // configure GlobePass
    GlobePassDesc globe_pass_desc = (GlobePassDesc) {
        .globe_radius = GLOBE_CONFIG.globe_radius,
        .globe_tex_offset = GLOBE_TEX_OFFSET,
        .shader_vert = &globe_vert,
        .shader_frag = &globe_frag,
        .path_globe_texture = "./assets/globe.bmp",
    };
    const GlobePass* const globe_pass = GlobePass_init(globe_pass_desc, globe_mesh);
    Globe_free(globe_mesh);
    if(globe_pass == NULL) abort();
#ifndef DISABLE_OVERLAY_UI
    // configure UI overlay
    OverlayDesc ui_desc = (OverlayDesc) {
        .font_path = "./assets/DejaVuSans.ttf",
        .font_size = 20,
        .text_color = { 0.f, 0.f, 0.f },
        .panel_color = { 0.6f, 0.2f, 0.2f },
    };
    OverlayUI* const ui = OverlayUI_init(ui_desc, window);
    if(ui == NULL) abort();
#endif
    // configure SchedulePass
    SchedulePassDesc skd_pass_desc = (SchedulePassDesc) {
        .color_ant = { (GLfloat) 1.f, (GLfloat) 0.f, (GLfloat) 0.f },
        .color_src = { (GLfloat) 1.f, (GLfloat) 1.f, (GLfloat) 1.f },
        .globe_radius = GLOBE_CONFIG.globe_radius,
        .shell_radius = GLOBE_CONFIG.globe_radius * CAMERA_CONFIG.scalar,
        .jd_inc = SKD_PASS_INCREMENT,
        .vert = &sched_vert,
        .frag = &sched_frag,
    };
#ifndef DISABLE_OVERLAY_UI
    SchedulePass* skd_pass = SchedulePass_init_from_schedule(skd_pass_desc, skd, ui);
#else
    SchedulePass* skd_pass = SchedulePass_init_from_schedule(skd_pass_desc, skd);
#endif
    if(skd_pass == NULL) abort();
    // event loop
    while (RGFW_window_shouldClose(window) == RGFW_FALSE) {
        while (RGFW_window_checkEvent(window)) {
            if(window->event.type == RGFW_windowResized) glViewport(0, 0, (GLsizei) window->r.w, (GLsizei) window->r.h);
            // process user input
            Camera_handle_events(camera, CAMERA_CONFIG, window);
            CameraController_handle_input(camera_controller, camera, window);
            // handle pausing/unpausing and resetting the visualization
            SchedulePass_handle_input(skd_pass, skd, window);
            // handle ui overlay events
#ifndef DISABLE_OVERLAY_UI
            OverlayUI_handle_events(ui, window);
#endif
        }
        // clear the display
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // update camera
        Camera_update(camera);
        // draw passes
        GlobePass_update_and_draw(globe_pass, camera);
#ifndef DISABLE_OVERLAY_UI
        SchedulePass_update_and_draw(skd_pass, skd, camera, ui, SKD_PASS_INCREMENT); // TODO: Tie dt to framerate
#else
        SchedulePass_update_and_draw(skd_pass, skd, camera, SKD_PASS_INCREMENT);
#endif
        // conclude pass
        RGFW_window_swapBuffers(window);
    }
    // clean up buffers
    Camera_free(camera);
    CameraController_free(camera_controller);
    GlobePass_free(globe_pass);
    Schedule_free(skd);
    SchedulePass_free(skd_pass);
#ifndef DISABLE_OVERLAY_UI
    OverlayUI_free(ui);
#endif
    // destroy shaders
    Shader_destroy(&globe_vert);
    Shader_destroy(&sched_vert);
    Shader_destroy(&sched_frag);
    Shader_destroy(&globe_frag);
    // close window
    RGFW_window_close(window);
    return 0;
}
