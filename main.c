#define LOGGING

#include <GL/glew.h>
#include <stdio.h>
#include <stdint.h>

#include "./include/stations.h"
#include "./include/skd.h"
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
    .slices = 32,\
    .stacks = 24,\
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
    Schedule skd;
    Schedule_build_from_source(&skd, "./schedules/r41192.skd"); // TODO
    Schedule_free(skd);
    unsigned int failure;
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
    // configure GlobePass
    GlobePassPaths globe_pass_paths = (GlobePassPaths) {
        .path_vert_shader = "./shaders/lat_lon.vs",
        .path_frag_shader = "./shaders/globe.fs",
        .path_globe_texture = "./assets/globe.bmp",
    };
    GlobePass globe_pass;
    failure = GlobePass_init(&globe_pass, globe_pass_paths, GLOBE_CONFIG);
    if(failure) abort();
    // configure StationPass
    StationPassDesc station_pass_desc = (StationPassDesc) {
        .globe_radius = GLOBE_CONFIG.rad,
        .shader_lat_lon = globe_pass.shader_vert,
        .path_frag_shader = "./shaders/fill_in.fs",
        .path_pos_catalog = "./assets/position.cat",
    };
    StationPass station_pass;
    failure = StationPass_init(&station_pass, station_pass_desc);
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
        StationPass_update_and_draw(station_pass, camera);
        // conclude pass
        RGFW_window_swapBuffers(window);
    }
    // clean up buffers
    GlobePass_free(globe_pass);
    StationPass_free(station_pass);
    // close window
    RGFW_window_close(window);
    return 0;
}