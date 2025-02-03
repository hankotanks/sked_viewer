#include <GL/glew.h>
#include <stdio.h>
#include <stdint.h>

#include "./include/globe.h"
#include "./include/util.h"
#include "./include/camera.h"

#define RGFW_IMPLEMENTATION
#include "./include/RGFW.h"

// WINDOW CONFIGURATION OPTIONS
#define WINDOW_TITLE "probe"
#define WINDOW_W 800
#define WINDOW_H 600
#define WINDOW_BOUNDS RGFW_RECT(0, 0, WINDOW_W, WINDOW_H)
// EARTH CONFIGURATION OPTIONS
#define EARTH_RAD 10.f
// CAMERA CONFIGURATION OPTIONS
#define CAMERA_FOV M_PI_2
#define CAMERA_Z_NEAR 0.1f
#define CAMERA_Z_FAR 100.f

int main() {
    // set up window
    RGFW_window* window = RGFW_createWindow(WINDOW_TITLE, WINDOW_BOUNDS, RGFW_windowCenter);
    glewInit(); // initialize GLEW
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    // configure camera and set aspect
    CameraCfg camera_cfg;
    CameraCfg_init(&camera_cfg, window, CAMERA_FOV, CAMERA_Z_NEAR, CAMERA_Z_FAR);
    Camera camera;
    Camera_init(&camera, EARTH_RAD * 2.f);
    Camera_perspective(&camera, camera_cfg);
    // set up earth texture
    BitmapImage earth_img;
    int failed = BitmapImage_load_from_file(&earth_img, "./assets/globe.bmp");
    if(failed) abort();
    GLuint earth_tex_id;
    BitmapImage_build_texture(earth_img, &earth_tex_id);
    // initialize earth mesh
    Globe earth = Globe_generate((GlobeProp) { .slices = 6, .stacks = 4, .rad = EARTH_RAD });
    // configure earth buffers
    GLuint VAO, VBO, EBO;
    Globe_configure_buffers(earth, &VAO, &VBO, &EBO);
    Globe_free(earth);
    // configure globe shaders
    const char* earth_vert_shader = read_file_contents("./shaders/globe.vs");
    if(earth_vert_shader == NULL) abort();
    const char* earth_frag_shader = read_file_contents("./shaders/globe.fs");
    if(earth_frag_shader == NULL) abort();
    GLuint earth_shader_program = compile_shader_program(earth_vert_shader, earth_frag_shader);
    if(!earth_shader_program) abort();
    free((char*) earth_vert_shader);
    free((char*) earth_frag_shader);
    glUseProgram(earth_shader_program);
    // pass the sampler for the earth texture
    GLuint samplerLoc = glGetUniformLocation(earth_shader_program, "sampler");
    glUniform1i(samplerLoc, 0);
    // event loop
    while (RGFW_window_shouldClose(window) == RGFW_FALSE) {
        while (RGFW_window_checkEvent(window)) {
            // adjust aspect if window size changed
            CameraCfg_handle_resize(&camera_cfg, window);
            // handle camera controls
            Camera_handle_input(&camera, window);
        }
        // update view and proj uniforms
        Camera_update(&camera, earth_shader_program);
        // clear the display
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glClear(GL_DEPTH_BUFFER_BIT);
        // draw the earth
        Globe_draw(earth, VAO);
        // conclude path
        RGFW_window_swapBuffers(window);
    }
    // clean up buffers
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    // close window
    RGFW_window_close(window);
    return 0;
}