#ifndef GLOBE_H
#define GLOBE_H

#include <GL/glew.h>
#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

typedef struct {
    GLfloat* vertices;
    GLuint* indices;
    size_t vertex_count;
    size_t index_count;
} Globe;

#define GLOBE_ERROR (Globe) {\
    .vertices = NULL,\
    .indices = NULL,\
    .vertex_count = 0,\
    .index_count = 0,\
}

void Globe_free(Globe mesh) {
    free(mesh.vertices);
    free(mesh.indices);
}

void Globe_configure_buffers(Globe mesh, GLuint* VAO, GLuint* VBO, GLuint* EBO) {
    glGenVertexArrays(1, VAO);
    glGenBuffers(1, VBO);
    glGenBuffers(1, EBO);
    glBindVertexArray(*VAO);
    glBindBuffer(GL_ARRAY_BUFFER, *VBO);
    glBufferData(GL_ARRAY_BUFFER, mesh.vertex_count * 3 * sizeof(GLfloat), mesh.vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.index_count * sizeof(GLuint), mesh.indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3, (GLvoid*) 0);
    glEnableVertexAttribArray(0);
}

void Globe_draw(Globe mesh, GLuint VAO) {
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, mesh.index_count * sizeof(GLuint), GL_UNSIGNED_INT, (GLvoid*) 0);
}

typedef struct {
    size_t slices;
    size_t stacks;
    float rad;
} GlobeProp;

Globe Globe_generate(GlobeProp cfg) {
    assert(cfg.stacks > 2 && cfg.slices > 2);
    size_t vertex_count = cfg.slices * (cfg.stacks) + 2;
    GLfloat* vertices = (GLfloat*) malloc(vertex_count * 3 * sizeof(GLfloat));
    if(vertices == NULL) return GLOBE_ERROR;
    size_t k_v = 0;
    vertices[k_v++] = (GLfloat) 0.f;
    vertices[k_v++] = (GLfloat) cfg.rad;
    vertices[k_v++] = (GLfloat) 0.f;
    for(size_t i = 0; i < (cfg.stacks); ++i) {
        float phi = M_PI * (float) (i + 1) / (float) cfg.stacks;
        for(size_t j = 0; j < cfg.slices; ++j) {
            float theta = M_PI * 2.f * (float) j / (float) cfg.slices;
            vertices[k_v++] = (GLfloat) (sinf(phi) * cosf(theta) * cfg.rad);
            vertices[k_v++] = (GLfloat) (cosf(phi) * cfg.rad);
            vertices[k_v++] = (GLfloat) (sinf(phi) * sinf(theta) * cfg.rad);
        }
    }
    vertices[k_v++] = (GLfloat) 0.f;
    vertices[k_v++] = (GLfloat) (cfg.rad * -1.f);
    vertices[k_v++] = (GLfloat) 0.f;
    size_t index_count = cfg.slices * 6 * (cfg.stacks - 1);
    GLuint* indices = (GLuint*) malloc(index_count * sizeof(GLuint));
    if(indices == NULL) return GLOBE_ERROR;
    size_t k_i = 0;
    GLuint slices_gl = (GLuint) cfg.slices;
    GLuint stacks_gl = (GLuint) cfg.stacks;
    for(GLuint i = 0; i < slices_gl; ++i) {
        GLuint i0 = i + 1;
        GLuint i1 = (i0 % slices_gl) + 1;
        indices[k_i++] = (GLuint) 0;
        indices[k_i++] = i1;
        indices[k_i++] = i0;
        i0 = i + slices_gl * (stacks_gl - 2) + 1;
        i1 = (i + 1) % slices_gl + slices_gl * (stacks_gl - 2) + 1;
        indices[k_i++] = (GLuint) (vertex_count - 1);
        indices[k_i++] = i0;
        indices[k_i++] = i1;
    }
    for(GLuint j = 0; j < (stacks_gl - 2); ++j) {
        GLuint j0 = j * slices_gl + 1;
        GLuint j1 = (j + 1) * slices_gl + 1;
        for(GLuint i = 0; i < slices_gl; ++i) {
            GLuint i0 = j0 + i;
            GLuint i1 = j0 + (i + 1) % slices_gl;
            GLuint i2 = j1 + (i + 1) % slices_gl;
            GLuint i3 = j1 + i;
            indices[k_i++] = i3; indices[k_i++] = i0; indices[k_i++] = i1;
            indices[k_i++] = i1; indices[k_i++] = i2; indices[k_i++] = i3;
        }
    }
    return (Globe) { 
        .vertices = vertices, 
        .indices = indices,
        .vertex_count = vertex_count, 
        .index_count = index_count,
    };
}

#endif /* GLOBE_H */