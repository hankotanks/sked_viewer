#ifndef GLOBE_H
#define GLOBE_H

#include <GL/glew.h>
#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include "util.h"

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

void Globe_draw(Globe mesh, GLuint program, GLuint VAO) {
    glUseProgram(program);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, mesh.index_count * sizeof(GLuint), GL_UNSIGNED_INT, (GLvoid*) 0);
    glUseProgram(0);
}

typedef struct {
    size_t slices;
    size_t stacks;
    float rad;
} GlobeProp;

void Globe_configure_buffers(Globe mesh, GlobeProp cfg, GLuint program, GLuint* VAO, GLuint* VBO, GLuint* EBO) {
    glGenVertexArrays(1, VAO);
    glGenBuffers(1, VBO);
    glGenBuffers(1, EBO);
    glBindVertexArray(*VAO);
    glBindBuffer(GL_ARRAY_BUFFER, *VBO);
    glBufferData(GL_ARRAY_BUFFER, mesh.vertex_count * 2 * sizeof(GLfloat), mesh.vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.index_count * sizeof(GLuint), mesh.indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 2, (GLvoid*) 0);
    glEnableVertexAttribArray(0);
    glUseProgram(program);
    glUniform1f(glGetUniformLocation(program, "globe_radius"), cfg.rad);
    // pass the sampler for the earth texture
    glUniform1i(glGetUniformLocation(program, "globe_sampler"), 0);
    glUseProgram(0);
}

int Globe_generate(Globe* mesh, GlobeProp cfg) {
    assert(cfg.stacks > 2 && cfg.slices > 2);
    mesh->vertex_count = cfg.slices * (cfg.stacks - 1) + 2;
    mesh->vertices = (GLfloat*) malloc(mesh->vertex_count * 2 * sizeof(GLfloat));
    if(mesh->vertices == NULL) {
        LOG_ERROR("Unable to allocate globe vertex buffer.");
        return 1;
    }
    size_t k_v = 0;
    mesh->vertices[k_v++] = (GLfloat) M_PI;
    mesh->vertices[k_v++] = (GLfloat) 0.f;
    for(size_t i = 0; i < (cfg.stacks - 1); ++i) {
        float phi = M_PI * (float) (i + 1) / (float) cfg.stacks;
        for(size_t j = 0; j < cfg.slices; ++j) {
            mesh->vertices[k_v++] = M_PI * 2.0 * (float) j / (float) cfg.slices;
            mesh->vertices[k_v++] = phi;
        }
    }
    mesh->vertices[k_v++] = (GLfloat) M_PI;
    mesh->vertices[k_v++] = (GLfloat) M_PI;
    mesh->index_count = cfg.slices * 6 * (cfg.stacks - 1);
    mesh->indices = (GLuint*) malloc(mesh->index_count * sizeof(GLuint));
    if(mesh->indices == NULL) {
        LOG_ERROR("Unable to allocate globe index buffer.");
        return 1;
    }
    size_t k_i = 0;
    GLuint slices_gl = (GLuint) cfg.slices;
    GLuint stacks_gl = (GLuint) cfg.stacks;
    for(GLuint i = 0; i < slices_gl; ++i) {
        GLuint i0 = i + 1;
        GLuint i1 = (i0 % slices_gl) + 1;
        mesh->indices[k_i++] = (GLuint) 0;
        mesh->indices[k_i++] = i1;
        mesh->indices[k_i++] = i0;
        i0 = i + slices_gl * (stacks_gl - 2) + 1;
        i1 = (i + 1) % slices_gl + slices_gl * (stacks_gl - 2) + 1;
        mesh->indices[k_i++] = (GLuint) (mesh->vertex_count - 1);
        mesh->indices[k_i++] = i0;
        mesh->indices[k_i++] = i1;
    }
    for(GLuint j = 0; j < (stacks_gl - 2); ++j) {
        GLuint j0 = j * slices_gl + 1;
        GLuint j1 = (j + 1) * slices_gl + 1;
        for(GLuint i = 0; i < slices_gl; ++i) {
            GLuint i0 = j0 + i;
            GLuint i1 = j0 + (i + 1) % slices_gl;
            GLuint i2 = j1 + (i + 1) % slices_gl;
            GLuint i3 = j1 + i;
            mesh->indices[k_i++] = i3; mesh->indices[k_i++] = i0; 
            mesh->indices[k_i++] = i1;
            mesh->indices[k_i++] = i1; mesh->indices[k_i++] = i2; 
            mesh->indices[k_i++] = i3;
        }
    }
    return 0;
}

#endif /* GLOBE_H */