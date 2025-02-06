#define LOGGING

#include <GL/glew.h>
#include <stdio.h>
#include <stdint.h>

#include "./include/util.h"
#include "./include/globe.h"
#include "./include/stations.h"
#include "./include/camera.h"

#define RGFW_IMPLEMENTATION
#include "./include/RGFW.h"

// WINDOW CONFIGURATION OPTIONS
#define WINDOW_TITLE "sked_viewer"
#define WINDOW_W 800
#define WINDOW_H 600
#define WINDOW_BOUNDS RGFW_RECT(0, 0, WINDOW_W, WINDOW_H)
// EARTH CONFIGURATION OPTIONS
#define EARTH_RAD 100.f // a = 6378.137f
// CAMERA CONFIGURATION OPTIONS
#define CAMERA_FOV M_PI_2
#define CAMERA_Z_NEAR 0.1f
#define CAMERA_Z_FAR 1000.f

int main() {
    unsigned int failure;
    // set up window
    RGFW_window* window = RGFW_createWindow(WINDOW_TITLE, WINDOW_BOUNDS, RGFW_windowCenter);
    glewInit(); // initialize GLEW
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glPointSize(5.f);
    glClearColor(0.f, 0.f, 0.f, 1.f);
    // configure camera and set aspect
    CameraController controller;
    CameraController_init(&controller, 0.005);
    Camera camera;
    Camera_init(&camera, EARTH_RAD, CAMERA_FOV, CAMERA_Z_NEAR, CAMERA_Z_FAR);
    Camera_set_aspect(&camera, window->r.w, window->r.h);
    Camera_perspective(&camera);
    // set up earth texture
    BitmapImage earth_img;
    failure = BitmapImage_load_from_file(&earth_img, "./assets/globe.bmp");
    if(failure) abort();
    glActiveTexture(GL_TEXTURE0);
    GLuint earth_tex_id;
    BitmapImage_build_texture(earth_img, &earth_tex_id);
    // initialize earth mesh
    GlobeProp earth_prop = (GlobeProp) { .slices = 32, .stacks = 24, .rad = EARTH_RAD };
    Globe earth;
    failure = Globe_generate(&earth, earth_prop);
    if(failure) abort();
    // configure globe shaders
    const char* dummy_vert_shader = read_file_contents("./shaders/dummy.vs");
    if(dummy_vert_shader == NULL) abort();
    const char* earth_frag_shader = read_file_contents("./shaders/globe.fs");
    if(earth_frag_shader == NULL) abort();
    GLuint earth_shader_program;
    failure = compile_shader_program(&earth_shader_program, dummy_vert_shader, earth_frag_shader);
    if(failure) abort();
    free((char*) earth_frag_shader);
    // configure earth buffers
    GLuint VAO_earth, VBO_earth, EBO_earth;
    Globe_configure_buffers(earth, earth_prop, earth_shader_program, &VAO_earth, &VBO_earth, &EBO_earth);
    Globe_free(earth);
    // configure stations
    Catalog cat = Catalog_parse_from_file("./assets/position.cat");
    if(cat.station_count == 0) abort();
    // configure station shaders
    const char* station_frag_shader = read_file_contents("./shaders/stations.fs");
    if(station_frag_shader == NULL) abort();
    GLuint station_shader_program;
    failure = compile_shader_program(&station_shader_program, dummy_vert_shader, station_frag_shader);
    if(failure) abort();
    free((char*) dummy_vert_shader);
    free((char*) station_frag_shader);
    // configure station position buffers
    GLuint VAO_station, VBO_station;
    Catalog_configure_buffers(cat, earth_prop.rad, station_shader_program, &VAO_station, &VBO_station);
    Catalog_free(cat);
    // event loop
    while (RGFW_window_shouldClose(window) == RGFW_FALSE) {
        while (RGFW_window_checkEvent(window)) {
            if(window->event.type == RGFW_windowResized) {
                glViewport(0, 0, (GLsizei) window->r.w, (GLsizei) window->r.h);
                // adjust aspect if window size changed
                Camera_set_aspect(&camera, window->r.w, window->r.h);
                Camera_perspective(&camera);
            }
            // process user input
            CameraController_handle_input(&controller, &camera, window);
        }
        // clear the display
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // update earth and draw
        Camera_update(&camera, earth_shader_program);
        Globe_draw(earth, earth_shader_program, VAO_earth);
        // update stations and draw
        Camera_update(&camera, station_shader_program);
        Catalog_draw(cat, station_shader_program, VAO_station);
        // conclude path
        RGFW_window_swapBuffers(window);
    }
    // clean up buffers
    glDeleteVertexArrays(1, &VAO_earth);
    glDeleteBuffers(1, &VBO_earth);
    glDeleteBuffers(1, &EBO_earth);
    glDeleteVertexArrays(1, &VAO_station);
    glDeleteBuffers(1, &VBO_station);
    // close window
    RGFW_window_close(window);
    return 0;
}