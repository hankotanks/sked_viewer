#ifndef GLOBE_H
#define GLOBE_H

#include <GL/glew.h>
#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include "util.h"
#include "camera.h"

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

typedef struct {
    size_t slices;
    size_t stacks;
    float rad;
} GlobeConfig;

unsigned int Globe_generate(Globe* mesh, GlobeConfig cfg) {
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

typedef struct {
    Globe mesh;
    GLuint shader_vert, shader_frag;
    GLuint shader_program;
    GLuint VAO, VBO, EBO;
} GlobePass;

void GlobePass_free(GlobePass pass) {
    Globe_free(pass.mesh);
    glDeleteShader(pass.shader_vert);
    glDeleteShader(pass.shader_frag);
    glDeleteProgram(pass.shader_program);
    glDeleteVertexArrays(1, &(pass.VAO));
    glDeleteBuffers(1, &(pass.VBO));
    glDeleteBuffers(1, &(pass.EBO));
}

void GlobePass_update_and_draw(GlobePass pass, Camera cam) {
    Camera_update_uniforms(cam, pass.shader_program);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glUseProgram(pass.shader_program);
    glBindVertexArray(pass.VAO);
    glDrawElements(GL_TRIANGLES, pass.mesh.index_count * sizeof(GLuint), GL_UNSIGNED_INT, (GLvoid*) 0);
    glUseProgram(0);
    glDisable(GL_DEPTH_TEST);
}

typedef struct {
    const char* path_vert_shader;
    const char* path_frag_shader;
    const char* path_globe_texture;
} GlobePassPaths;

unsigned int GlobePass_init(GlobePass* pass, GlobePassPaths paths, GlobeConfig cfg) {
    unsigned int failure;
    BitmapImage globe_texture;
    failure = BitmapImage_load_from_file(&globe_texture, paths.path_globe_texture);
    if(failure) return 1;
    GLuint globe_texture_id;
    BitmapImage_build_texture(globe_texture, &globe_texture_id, GL_TEXTURE0);
    // initialize earth mesh
    Globe mesh;
    failure = Globe_generate(&mesh, cfg);
    if(failure) {
        BitmapImage_free(globe_texture);
        return 1;
    }
    // configure earth shaders
    GLuint vert, frag;
    failure = compile_shader(&vert, GL_VERTEX_SHADER, paths.path_vert_shader);
    if(failure) {
        BitmapImage_free(globe_texture);
        return 1;
    }
    failure = compile_shader(&frag, GL_FRAGMENT_SHADER, paths.path_frag_shader);
    if(failure) {
        BitmapImage_free(globe_texture);
        glDeleteShader(vert);
    }
    GLuint shader_program;
    failure = assemble_shader_program(&shader_program, vert, frag);
    if(failure) {
        BitmapImage_free(globe_texture);
        glDeleteShader(vert);
        glDeleteShader(frag);
        return 1;
    };
    pass->shader_vert = vert;
    pass->shader_frag = frag;
    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, mesh.vertex_count * 2 * sizeof(GLfloat), mesh.vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.index_count * sizeof(GLuint), mesh.indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 2, (GLvoid*) 0);
    glEnableVertexAttribArray(0);
    glUseProgram(shader_program);
    glUniform1f(glGetUniformLocation(shader_program, "globe_radius"), cfg.rad);
    // pass the sampler for the earth texture
    glActiveTexture(globe_texture_id);
    glUniform1i(glGetUniformLocation(shader_program, "globe_sampler"), 0);
    glActiveTexture(0);
    glUseProgram(0);
    BitmapImage_free(globe_texture);
    pass->VAO = VAO;
    pass->VBO = VBO;
    pass->EBO = EBO;
    pass->shader_program = shader_program;
    pass->mesh = mesh;
    return 0;
}

#endif /* GLOBE_H */