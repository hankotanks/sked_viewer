#ifndef CAMERA_H
#define CAMERA_H

#include <stdint.h>
#include <GL/glew.h>
#include "RGFW_common.h"

// contains the required information to generate proj/view matrices
typedef struct __CAMERA_H__Camera Camera;
// configurations for Camera
typedef struct {
    float scalar;
    float fov;
    float z_near, z_far;
} CameraConfig;
// initialize Camera
Camera* Camera_init(CameraConfig cfg, float globe_radius);
// free Camera
void Camera_free(Camera* cam);
// update Camera view matrix
void Camera_update(Camera* const cam);
// both functions should be called on window resize
void Camera_set_aspect(Camera* const cam, const RGFW_window* const win);
void Camera_perspective(Camera* const cam, CameraConfig cfg);
// update proj and view mat4 uniforms in given shader program
unsigned int Camera_update_uniforms(const Camera* const cam, GLuint shader_program);
// handles control of
typedef struct __CAMERA_H__CameraController CameraController;
// initialize CameraController
CameraController* CameraController_init(float sensitivity);
// free CameraController
void CameraController_free(CameraController* cont);
// apply user events to Camera
void CameraController_handle_input(CameraController* const cont, Camera* const cam, const RGFW_window* const win);

#endif /* CAMERA_H */
