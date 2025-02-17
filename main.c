#define LOGGING
// #define DRAW_POSITION_CATLOG

#include <GL/glew.h>
#include <stdio.h>
#include <stdint.h>

#include "./include/globe.h"
#include "./include/camera.h"
#include "./include/skd.h"
#include "./include/skd_pass.h"

#ifndef RGFW_IMPLEMENTATION
#define RGFW_IMPLEMENTATION
#endif
#include "./include/RGFW.h"

// window configuration options
#define WINDOW_TITLE "sked_viewer"
#define WINDOW_BOUNDS RGFW_RECT(0, 0, 800, 600)
// earth configuration options
#define GLOBE_TEX_OFFSET 4.94f
#define GLOBE_CONFIG (GlobeConfig) {\
    .slices = 64,\
    .stacks = 48,\
    .globe_radius = 100.f,\
}
// camera configuration options
#define CAMERA_SENSITIVITY 0.002
#define CAMERA_SCALAR 4.f
#define CAMERA_CONFIG (CameraConfig) {\
    .scalar = CAMERA_SCALAR,\
    .fov = M_PI_2,\
    .z_near = 1.f,\
    .z_far = CAMERA_SCALAR * GLOBE_CONFIG.globe_radius * 2.f,\
}

int main() {
    unsigned int failure;
    // build and validate Schedule
    Schedule skd; // TODO: Still working on the Schedule parsing
    Schedule_build_from_source(&skd, "./assets/r41192.skd"); // TODO
    failure = Schedule_debug_and_validate(skd, 0);
    if(failure) abort();
    // set up window
    RGFW_window* window = RGFW_createWindow(WINDOW_TITLE, WINDOW_BOUNDS, RGFW_windowCenter);
    glewInit();
    glClearColor(0.f, 0.f, 0.f, 1.f);
    // configure camera and set aspect
    CameraController camera_controller;
    CameraController_init(&camera_controller, CAMERA_SENSITIVITY);
    Camera camera;
    Camera_init(&camera, CAMERA_CONFIG, GLOBE_CONFIG.globe_radius);
    Camera_set_aspect(&camera, window->r.w, window->r.h);
    Camera_perspective(&camera, CAMERA_CONFIG);
    // set up shaders
    Shader globe_vert, sched_vert, globe_frag, sched_frag;
    globe_vert = Shader_init("./shaders/globe.vs", GL_VERTEX_SHADER);
    sched_vert = Shader_init("./shaders/sched.vs", GL_VERTEX_SHADER);
    globe_frag = Shader_init("./shaders/globe.fs", GL_FRAGMENT_SHADER);
    sched_frag = Shader_init("./shaders/sched.fs", GL_FRAGMENT_SHADER);
    // configure GlobePass
    GlobePassDesc globe_pass_desc = (GlobePassDesc) {
        .globe_tex_offset = GLOBE_TEX_OFFSET,
        .shader_vert = &globe_vert,
        .shader_frag = &globe_frag,
        .path_globe_texture = "./assets/globe.bmp",
    };
    GlobePass globe_pass;
    failure = GlobePass_init(&globe_pass, globe_pass_desc, GLOBE_CONFIG);
    if(failure) abort();
    // configure SchedulePass
    SchedulePassDesc skd_pass_desc = (SchedulePassDesc) {
        .color_ant = { (GLfloat) 1.f, (GLfloat) 0.f, (GLfloat) 0.f },
        .color_src = { (GLfloat) 1.f, (GLfloat) 1.f, (GLfloat) 1.f },
        .globe_radius = GLOBE_CONFIG.globe_radius,
        .shell_radius = GLOBE_CONFIG.globe_radius * CAMERA_CONFIG.scalar,
        .vert = &sched_vert,
        .frag = &sched_frag,
    };
    SchedulePass skd_pass;
    failure = SchedulePass_init_from_schedule(&skd_pass, skd_pass_desc, skd);
    if(failure) abort();
    // event loop
    while (RGFW_window_shouldClose(window) == RGFW_FALSE) {
        while (RGFW_window_checkEvent(window)) {
            switch(window->event.type) {
                case RGFW_windowResized:
                    glViewport(0, 0, (GLsizei) window->r.w, (GLsizei) window->r.h);
                    // adjust aspect if window size changed
                    Camera_set_aspect(&camera, window->r.w, window->r.h);
                    Camera_perspective(&camera, CAMERA_CONFIG);
                    break;
                case RGFW_keyPressed:
                    // see if the current observation was advanced
                    if(window->event.key == RGFW_space) 
                        SchedulePass_next_observation(&skd_pass, skd, 1);
                default: break;
            }
            // process user input
            CameraController_handle_input(&camera_controller, &camera, window);
        }
        // clear the display
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // update camera
        Camera_update(&camera);
        // draw passes
        GlobePass_update_and_draw(globe_pass, camera);
        SchedulePass_update_and_draw(skd_pass, camera);
        // conclude pass
        RGFW_window_swapBuffers(window);
    }
    // clean up buffers
    GlobePass_free(globe_pass);
    Schedule_free(skd);
    SchedulePass_free(skd_pass);
    Shader_destroy(&globe_vert);
    Shader_destroy(&sched_vert);
    Shader_destroy(&sched_frag);
    Shader_destroy(&globe_frag);
    // close window
    RGFW_window_close(window);
    return 0;
}