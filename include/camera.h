#ifndef CAMERA_H
#define CAMERA_H

#include <GL/glew.h>
#include <math.h>

#include "RGFW.h"

#include "lalg.h"

typedef struct {
    float azi;
    float ele;
    float rad;
    GLfloat proj[16];
    GLfloat view[16];
} Camera;

void Camera_init(Camera* cam, float rad) {
    cam->azi = 0.f;
    cam->ele = 0.f;
    cam->rad = rad * 2;
    for(size_t i = 0; i < 16; ++i) {
        cam->proj[i] = (GLfloat) 0.f;
        cam->view[i] = (GLfloat) 0.f;
    }
}

void Camera_update(Camera* cam, GLuint program) {
    GLfloat eye[3];
    eye[0] = (GLfloat) (cam->rad * cosf(cam->ele) * sinf(cam->azi));
    eye[1] = (GLfloat) (cam->rad * sinf(cam->ele));
    eye[2] = (GLfloat) (cam->rad * cosf(cam->ele) * cosf(cam->azi));

    GLfloat up[3];
    up[0] = 0.f; up[1] = 1.f; up[2] = 0.f;

    look_at(cam->view, eye, up);

    GLint proj_loc = glGetUniformLocation(program, "proj");
    glUniformMatrix4fv(proj_loc, 1, GL_FALSE, cam->proj);
    GLint view_loc = glGetUniformLocation(program, "view");
    glUniformMatrix4fv(view_loc, 1, GL_FALSE, cam->view);
}

void Camera_handle_input(Camera* cam, RGFW_window* win) {
    if(RGFW_isPressed(win, 'w')) cam->ele += 0.05f;
    if(RGFW_isPressed(win, 's')) cam->ele -= 0.05f;
    if(cam->ele > M_PI_2) cam->ele = M_PI_2;
    if(cam->ele < M_PI_2 * -1.0) cam->ele = M_PI_2 * -1.0;
    if(RGFW_isPressed(win, 'a')) cam->azi -= 0.05f;
    if(RGFW_isPressed(win, 'd')) cam->azi += 0.05f;
    if(cam->rad < 1.0) cam->rad = 1.0;    
}

typedef struct {
    float fov;
    float aspect;
    float z_near;
    float z_far;
} CameraCfg;

void CameraCfg_init(CameraCfg* cfg, RGFW_window* win, float fov, float z_near, float z_far) {
    cfg->fov = fov;
    cfg->aspect = ((float) win->r.w) / ((float) win->r.h);
    cfg->z_near = z_near;
    cfg->z_far = z_far;
}

void CameraCfg_handle_resize(CameraCfg* cfg, RGFW_window* win) {
    if(win->event.type == RGFW_windowResized) {
        cfg->aspect = ((float) win->r.w) / ((float) win->r.h);
    }
}

void Camera_perspective(Camera* cam, CameraCfg cfg) {
    GLfloat temp = tanf(cfg.fov / 2.f);
    for(size_t i = 0; i < 16; ++i) cam->proj[i] = (GLfloat) 0.f;
    cam->proj[ 0] = (GLfloat) (1.f / (cfg.aspect * temp)); 
    cam->proj[ 5] = (GLfloat) (1.f / temp);
    cam->proj[10] = (GLfloat) ((cfg.z_far + cfg.z_near) / (cfg.z_far - cfg.z_near) * -1.f); 
    cam->proj[11] = (GLfloat) (-1.f);
    cam->proj[14] = (GLfloat) ((2.f * cfg.z_far * cfg.z_near) / (cfg.z_far - cfg.z_near) * -1.f); 
}

#endif /* CAMERA_H */