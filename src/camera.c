#include "camera.h"
#include <stdint.h>
#include <math.h>
#include <GL/glew.h>
#include "RGFW_common.h"
#include "util/log.h"
#include "util/lalg.h"

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

struct __CAMERA_H__Camera {
    float azi, ele, rad;
    float min, max;
    float aspect;
    GLfloat proj[16];
    GLfloat view[16];
};

Camera* Camera_init(CameraConfig cfg, float globe_radius) {
    Camera* cam = (Camera*) malloc(sizeof(Camera));
    if(cam == NULL) {
        LOG_ERROR("Failed to allocate Camera.");
        return NULL;
    }
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
    return cam;
}

void Camera_free(Camera* cam) {
    free(cam);
}

void Camera_update(Camera* const cam) {
    GLfloat eye[3];
    eye[0] = (GLfloat) (cam->rad * cosf(cam->ele) * sinf(cam->azi));
    eye[1] = (GLfloat) (cam->rad * sinf(cam->ele));
    eye[2] = (GLfloat) (cam->rad * cosf(cam->ele) * cosf(cam->azi));
    GLfloat up[3];
    up[0] = 0.f; up[1] = 1.f; up[2] = 0.f;
    look_at(cam->view, eye, up);
}

void Camera_set_aspect(Camera* const cam, const RGFW_window* const win) {
    float w, h;
    w = (float) (win->r.w);
    h = (float) (win->r.h);
    cam->aspect = w / h;
}

void Camera_perspective(Camera* const cam, CameraConfig cfg) {
    GLfloat temp = tanf(cfg.fov / 2.f);
    for(size_t i = 0; i < 16; ++i) cam->proj[i] = (GLfloat) 0.f;
    cam->proj[ 0] = (GLfloat) (1.f / (cam->aspect * temp)); 
    cam->proj[ 5] = (GLfloat) (1.f / temp);
    cam->proj[10] = (GLfloat) ((cfg.z_far + cfg.z_near) / (cfg.z_far - cfg.z_near) * -1.f); 
    cam->proj[11] = (GLfloat) (-1.f);
    cam->proj[14] = (GLfloat) ((2.f * cfg.z_far * cfg.z_near) / (cfg.z_far - cfg.z_near) * -1.f); 
}

unsigned int Camera_update_uniforms(const Camera* const cam, GLuint shader_program) {
    GLint loc;
    glUseProgram(shader_program);
    loc = glGetUniformLocation(shader_program, "proj");
    if(loc == -1) {
        LOG_ERROR("Unable to update 'proj' uniform in given shader program.");
        return 1;
    }
    glUniformMatrix4fv(loc, 1, GL_FALSE, cam->proj);
    loc = glGetUniformLocation(shader_program, "view");
    if(loc == -1) {
        LOG_ERROR("Unable to update 'view' uniform in given shader program.");
        return 1;
    }
    glUniformMatrix4fv(loc, 1, GL_FALSE, cam->view);
    glUseProgram(0);
    return 0;
}

struct __CAMERA_H__CameraController {
    uint16_t initialized, dragging;
    float sensitivity;
    int32_t mouse_pos_x, mouse_pos_y;
};

CameraController* CameraController_init(float sensitivity) {
    CameraController* cont = (CameraController*) calloc(1, sizeof(CameraController));
    if(cont == NULL) {
        LOG_ERROR("Failed to allocate CameraController.");
        return NULL;
    }
    cont->sensitivity = sensitivity;
    return cont;
}

void CameraController_free(CameraController* cont) {
    free(cont);
}

void CameraController_handle_input(CameraController* const cont, Camera* const cam, const RGFW_window* const win) {
    int32_t x, y, dx, dy;
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
            x = win->event.point.x;
            y = win->event.point.y;
            if(cont->dragging && cont->initialized) {
                dx = x - cont->mouse_pos_x;
                dy = y - cont->mouse_pos_y;
                cam->azi -= cont->sensitivity * (float) dx;
                cam->ele += cont->sensitivity * (float) dy;
                if(cam->ele > M_PI_2 * 0.9) cam->ele = M_PI_2 * 0.9;
                if(cam->ele < M_PI_2 * -0.9) cam->ele = M_PI_2 * -0.9;
            }
            cont->mouse_pos_x = x;
            cont->mouse_pos_y = y;
            cont->initialized = 1;
            break;
        default: break;
    }
}
