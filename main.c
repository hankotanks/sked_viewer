#define LOGGING
// #define DRAW_POSITION_CATLOG

#include <GL/glew.h>
#include <stdio.h>
#include <stdint.h>

#include "./include/globe.h"
#include "./include/camera.h"
#include "./include/skd.h"
#include "./include/stations.h"
#include "./include/sources.h"

#ifndef RGFW_IMPLEMENTATION
#define RGFW_IMPLEMENTATION
#endif
#include "./include/RGFW.h"

// window configuration options
#define WINDOW_TITLE "sked_viewer"
#define WINDOW_BOUNDS RGFW_RECT(0, 0, 800, 600)
// earth configuration options
#define GLOBE_LAM_OFFSET 4.94f
#define GLOBE_CONFIG (GlobeConfig) {\
    .slices = 64,\
    .stacks = 48,\
    .globe_radius = 100.f,\
}
// camera configuration options
#define CAMERA_CONTROLLER_SENSITIVITY 0.005
#define CAMERA_CONFIG (CameraConfig) {\
    .fov = M_PI_2,\
    .z_near = 0.1,\
    .z_far = 1000.f,\
}

int main() {
    Datetime temp = { .yrs = 2025, .day = 1, .hrs = 0, .min = 0, .sec = 0 };
    Datetime_greenwich_sidereal_time(temp);
    temp = (Datetime) { .yrs = 2025, .day = 4, .hrs = 0, .min = 0, .sec = 0 };
    Datetime_greenwich_sidereal_time(temp);
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
    CameraController_init(&camera_controller, CAMERA_CONTROLLER_SENSITIVITY);
    Camera camera;
    Camera_init(&camera, GLOBE_CONFIG.globe_radius);
    Camera_set_aspect(&camera, window->r.w, window->r.h);
    Camera_perspective(&camera, CAMERA_CONFIG);
    // set up shaders
    Shader shader_lam_phi, shader_alf_phi, shader_fill_in, shader_basemap;
    shader_lam_phi = Shader_init("./shaders/lam_phi.vs", GL_VERTEX_SHADER);
    shader_alf_phi = Shader_init("./shaders/alf_phi.vs", GL_VERTEX_SHADER);
    shader_fill_in = Shader_init("./shaders/fill_in.fs", GL_FRAGMENT_SHADER);
    shader_basemap = Shader_init("./shaders/basemap.fs", GL_FRAGMENT_SHADER);
    // configure GlobePass
    GlobePassDesc globe_pass_desc = (GlobePassDesc) {
        .globe_lam_offset = GLOBE_LAM_OFFSET,
        .shader_vert = &shader_lam_phi,
        .shader_frag = &shader_basemap,
        .path_globe_texture = "./assets/globe.bmp",
    };
    GlobePass globe_pass;
    failure = GlobePass_init(&globe_pass, globe_pass_desc, GLOBE_CONFIG);
    if(failure) abort();
    // configure SourcePass
    SchedPassDesc source_pass_desc = (SchedPassDesc) {
        .color = { 0.f, 1.f, 0.f },
        .shader_vert = &shader_alf_phi,
        .shader_frag = &shader_fill_in,
    };
    SourcePass source_pass;
    failure = SourcePass_build(&source_pass, source_pass_desc, skd);
    if(failure) abort();
    GlobeConfig_set_globe_radius_uniform(GLOBE_CONFIG, 2.f, source_pass.shader_program);
    // configure StationPass
    SchedPassDesc station_pass_desc = (SchedPassDesc) {
        .color = { 1.f, 0.f, 0.f },
        .shader_vert = &shader_lam_phi,
        .shader_frag = &shader_fill_in,
    };
    StationPass station_pass;
#ifndef DRAW_POSITION_CATLOG
    failure = StationPass_build_from_schedule(&station_pass, station_pass_desc, skd);
#else
    failure = StationPass_build_from_catalog(&station_pass, station_pass_desc, "./assets/position.cat");
#endif
    if(failure) abort();
    GlobeConfig_set_globe_radius_uniform(GLOBE_CONFIG, 1.f, station_pass.shader_program);
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
                    if(window->event.key == RGFW_space) SourcePass_next(&source_pass, skd);
                default: break;
            }
            // process user input
            CameraController_handle_input(&camera_controller, &camera, GLOBE_CONFIG.globe_radius, window);
        }
        // clear the display
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // update camera
        Camera_update(&camera);
        // draw passes
        GlobePass_update_and_draw(globe_pass, camera);
        SourcePass_update_and_draw(source_pass, camera);
        StationPass_update_and_draw(station_pass, camera);
        // conclude pass
        RGFW_window_swapBuffers(window);
    }
    // clean up buffers
    GlobePass_free(globe_pass);
    Schedule_free(skd);
    StationPass_free(station_pass);
    SourcePass_free(source_pass);
    Shader_destroy(&shader_lam_phi);
    Shader_destroy(&shader_fill_in);
    Shader_destroy(&shader_basemap);
    // close window
    RGFW_window_close(window);
    return 0;
}