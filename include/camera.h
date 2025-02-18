#ifndef CAMERA_H
#define CAMERA_H

#include <GL/glew.h>
#include <math.h>
#include "RGFW.h"
#include "util/lalg.h"

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

#define CAMERA_ELE_SCALAR 0.9

typedef struct {
    float azi, ele, rad;
    float min, max;
    float aspect;
    GLfloat proj[16];
    GLfloat view[16];
} Camera;

typedef struct {
    float scalar;
    float fov;
    float z_near, z_far;
} CameraConfig;

void Camera_init(Camera* cam, CameraConfig cfg, float globe_radius) {
    cam->azi = 0.f;
    cam->ele = 0.f;
    cam->rad = globe_radius * (cfg.scalar - 1.f);
    cam->aspect = 1.f;
    cam->min = globe_radius;
    cam->max = globe_radius * cfg.scalar;
    for(size_t i = 0; i < 16; ++i) {
        cam->proj[i] = (GLfloat) 0.f;
        cam->view[i] = (GLfloat) 0.f;
    }
}

void Camera_update(Camera* cam) {
    GLfloat eye[3];
    eye[0] = (GLfloat) (cam->rad * cosf(cam->ele) * sinf(cam->azi));
    eye[1] = (GLfloat) (cam->rad * sinf(cam->ele));
    eye[2] = (GLfloat) (cam->rad * cosf(cam->ele) * cosf(cam->azi));
    GLfloat up[3];
    up[0] = 0.f; up[1] = 1.f; up[2] = 0.f;
    look_at(cam->view, eye, up);
}

void Camera_update_uniforms(Camera cam, GLuint program) {
    glUseProgram(program);
    GLint proj_loc = glGetUniformLocation(program, "proj");
    glUniformMatrix4fv(proj_loc, 1, GL_FALSE, cam.proj);
    GLint view_loc = glGetUniformLocation(program, "view");
    glUniformMatrix4fv(view_loc, 1, GL_FALSE, cam.view);
    glUseProgram(0);
}

void Camera_set_aspect(Camera* cam, int32_t w, int32_t h) {
    cam->aspect = (float) w / (float) h;
}

void Camera_perspective(Camera* cam, CameraConfig cfg) {
    GLfloat temp = tanf(cfg.fov / 2.f);
    for(size_t i = 0; i < 16; ++i) cam->proj[i] = (GLfloat) 0.f;
    cam->proj[ 0] = (GLfloat) (1.f / (cam->aspect * temp)); 
    cam->proj[ 5] = (GLfloat) (1.f / temp);
    cam->proj[10] = (GLfloat) ((cfg.z_far + cfg.z_near) / (cfg.z_far - cfg.z_near) * -1.f); 
    cam->proj[11] = (GLfloat) (-1.f);
    cam->proj[14] = (GLfloat) ((2.f * cfg.z_far * cfg.z_near) / (cfg.z_far - cfg.z_near) * -1.f); 
}

typedef struct {
    uint16_t initialized, dragging;
    float sensitivity;
    int32_t mouse_pos_x, mouse_pos_y;
} CameraController;

void CameraController_init(CameraController* cont, float sensitivity) {
    cont->sensitivity = sensitivity;
}

void CameraController_handle_input(CameraController* cont, Camera* cam, RGFW_window* win) {
    switch(win->event.type) {
        case RGFW_mouseButtonPressed:
            cont->dragging = 1;
            float rad_vel = cam->min * sqrtf(cont->sensitivity) * 2.f;
            float rad_min = cam->min + rad_vel;
            switch(win->event.button) {
                case RGFW_mouseScrollUp:
                    cam->rad = MAX(rad_min, cam->rad - rad_vel);
                    break;
                case RGFW_mouseScrollDown:
                    cam->rad = MIN(cam->max, cam->rad + rad_vel);
                    break;
                default: break;
            }
            break;
        case RGFW_mouseButtonReleased:
            cont->dragging = 0;
            break;
        case RGFW_mousePosChanged:
            int32_t x = win->event.point.x;
            int32_t y = win->event.point.y;
            if(cont->dragging && cont->initialized) {
                int32_t dx = x - cont->mouse_pos_x;
                int32_t dy = y - cont->mouse_pos_y;
                cam->azi -= cont->sensitivity * (float) dx;
                cam->ele += cont->sensitivity * (float) dy;
                if(cam->ele > M_PI_2 *  CAMERA_ELE_SCALAR) cam->ele = M_PI_2 *  CAMERA_ELE_SCALAR;
                if(cam->ele < M_PI_2 * -CAMERA_ELE_SCALAR) cam->ele = M_PI_2 * -CAMERA_ELE_SCALAR;
            }
            cont->mouse_pos_x = x;
            cont->mouse_pos_y = y;
            cont->initialized = 1;
            break;
        default: break;
    }
}

#endif /* CAMERA_H */