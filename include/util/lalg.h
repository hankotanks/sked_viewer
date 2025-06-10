#ifndef __LALG_H__
#define __LALG_H__

#include <GL/glew.h>
#include <math.h>

#pragma GCC diagnostic ignored "-Wunused-function"
static inline float mag(const GLfloat* const vec) {
    return sqrtf((float) (vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2]));
}

#pragma GCC diagnostic ignored "-Wunused-function"
static void normalize(GLfloat* const vec) {
    float length = mag(vec);
    if(length > 0.f) {
        vec[0] /= (GLfloat) length;
        vec[1] /= (GLfloat) length;
        vec[2] /= (GLfloat) length;
    }
}

#pragma GCC diagnostic ignored "-Wunused-function"
static void cross(GLfloat* const out, const GLfloat* const a, const GLfloat* const b) {
    out[0] = a[1] * b[2] - a[2] * b[1];
    out[1] = a[2] * b[0] - a[0] * b[2];
    out[2] = a[0] * b[1] - a[1] * b[0];
}

#pragma GCC diagnostic ignored "-Wunused-function"
static inline float dot(const GLfloat* const a, const GLfloat* const b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

#pragma GCC diagnostic ignored "-Wunused-function"
static void look_at(GLfloat* const view, GLfloat* eye, GLfloat* up) {
    GLfloat f[3], s[3], u[3];
    f[0] = eye[0] * -1.f;
    f[1] = eye[1] * -1.f;
    f[2] = eye[2] * -1.f;
    normalize(f);
    cross(s, f, up);
    normalize(s);
    cross(u, s, f);
    view[0] = s[0]; view[1] = u[0]; view[ 2] = -f[0]; view[ 3] = 0.f;
    view[4] = s[1]; view[5] = u[1]; view[ 6] = -f[1]; view[ 7] = 0.f;
    view[8] = s[2]; view[9] = u[2]; view[10] = -f[2]; view[11] = 0.f;
    view[12] = -dot(s, eye); 
    view[13] = -dot(u, eye); 
    view[14] = dot(f, eye); view[15] = 1.f;
}

#endif /* __LALG_H__ */
