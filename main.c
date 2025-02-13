#define LOGGING
#define DISABLE_STATION_PASS

#include <GL/glew.h>
#include <stdio.h>
#include <stdint.h>

#include "./include/stations.h"
#include "./include/skd.h"
#include "./include/skd_render.h"
#include "./include/globe.h"
#include "./include/camera.h"

#ifndef RGFW_IMPLEMENTATION
#define RGFW_IMPLEMENTATION
#endif
#include "./include/RGFW.h"

// window configuration options
#define WINDOW_TITLE "sked_viewer"
#define WINDOW_BOUNDS RGFW_RECT(0, 0, 800, 600)
// earth configuration options
#define GLOBE_CONFIG (GlobeConfig) {\
    .slices = 64,\
    .stacks = 48,\
    .rad = 100.f,\
}
// camera configuration options
#define CAMERA_CONTROLLER_SENSITIVITY 0.005
#define CAMERA_CONFIG (CameraConfig) {\
    .fov = M_PI_2,\
    .z_near = 0.1,\
    .z_far = 1000.f,\
}

int main() {
    unsigned int failure;
    // build and validate Schedule
    Schedule skd; // TODO: Still working on the Schedule parsing
    Schedule_build_from_source(&skd, "./assets/r41192.skd"); // TODO
    failure = Schedule_debug_and_validate(skd, 1);
    if(failure) abort();
    // set up window
    RGFW_window* window = RGFW_createWindow(WINDOW_TITLE, WINDOW_BOUNDS, RGFW_windowCenter);
    glewInit();
    glClearColor(0.f, 0.f, 0.f, 1.f);
    // configure camera and set aspect
    CameraController camera_controller;
    CameraController_init(&camera_controller, CAMERA_CONTROLLER_SENSITIVITY);
    Camera camera;
    Camera_init(&camera, GLOBE_CONFIG.rad);
    Camera_set_aspect(&camera, window->r.w, window->r.h);
    Camera_perspective(&camera, CAMERA_CONFIG);
    // set up shaders
    Shader shader_lat_lon, shader_fill_in, shader_basemap;
    shader_lat_lon = Shader_init("./shaders/lat_lon.vs", GL_VERTEX_SHADER);
    shader_fill_in = Shader_init("./shaders/fill_in.fs", GL_FRAGMENT_SHADER);
    shader_basemap = Shader_init("./shaders/basemap.fs", GL_FRAGMENT_SHADER);
    // configure GlobePass
    GlobePassDesc globe_pass_desc = (GlobePassDesc) {
        .shader_vert = &shader_lat_lon,
        .shader_frag = &shader_basemap,
        .path_globe_texture = "./assets/globe.bmp",
    };
    GlobePass globe_pass;
    failure = GlobePass_init(&globe_pass, globe_pass_desc, GLOBE_CONFIG);
    if(failure) abort();
#ifndef DISABLE_STATION_PASS
    // configure StationPass
    StationPassDesc station_pass_desc = (StationPassDesc) {
        .globe_radius = GLOBE_CONFIG.rad,
        .shader_vert = &shader_lat_lon,
        .shader_frag = &shader_fill_in,
        .path_pos_catalog = "./assets/position.cat",
    };
    StationPass station_pass;
    failure = StationPass_init(&station_pass, station_pass_desc);
    if(failure) abort();
#endif
    // configure SchedulePass
    SchedulePassDesc skd_pass_desc = (SchedulePassDesc) {
        .globe_radius = GLOBE_CONFIG.rad,
        .shader_vert = &shader_lat_lon,
        .shader_frag = &shader_fill_in,
    };
    SchedulePass skd_pass;
    failure = SchedulePass_init(&skd_pass, skd_pass_desc, &skd);
    if(failure) abort();
    // event loop
    while (RGFW_window_shouldClose(window) == RGFW_FALSE) {
        while (RGFW_window_checkEvent(window)) {
            if(window->event.type == RGFW_windowResized) {
                glViewport(0, 0, (GLsizei) window->r.w, (GLsizei) window->r.h);
                // adjust aspect if window size changed
                Camera_set_aspect(&camera, window->r.w, window->r.h);
                Camera_perspective(&camera, CAMERA_CONFIG);
            }
            // process user input
            CameraController_handle_input(&camera_controller, &camera, GLOBE_CONFIG.rad, window);
        }
        // clear the display
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // update camera
        Camera_update(&camera);
        // draw passes
        GlobePass_update_and_draw(globe_pass, camera);
#ifndef DISABLE_STATION_PASS
        StationPass_update_and_draw(station_pass, camera);
#endif
        SchedulePass_update_and_draw(skd_pass, camera);
        // conclude pass
        RGFW_window_swapBuffers(window);
    }
    // clean up buffers
    GlobePass_free(globe_pass);
#ifndef DISABLE_STATION_PASS
    StationPass_free(station_pass);
#endif
    Schedule_free(skd);
    SchedulePass_free(skd_pass);
    Shader_destroy(&shader_lat_lon);
    Shader_destroy(&shader_fill_in);
    Shader_destroy(&shader_basemap);
    // close window
    RGFW_window_close(window);
    return 0;
}