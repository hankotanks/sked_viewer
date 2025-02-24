#include "globe.h"
#include <GL/glew.h>
#include <assert.h>
#include "util/log.h"
#include "util/shaders.h"
#include "util/bmp.h"

struct __GLOBE_H__Globe {
    GLfloat* vertices;
    GLuint* indices;
    size_t vertex_count, index_count;
};

const Globe* Globe_generate(GlobeConfig cfg) {
    assert(cfg.stacks > 2 && cfg.slices > 2);
    size_t vertex_count = cfg.slices * (cfg.stacks - 1) + 2;
    GLfloat* vertices = (GLfloat*) malloc(vertex_count * 2 * sizeof(GLfloat));
    if(vertices == NULL) {
        LOG_ERROR("Unable to allocate globe vertex buffer.");
        return NULL;
    }
    size_t k_v = 0;
    vertices[k_v++] = (GLfloat) M_PI;
    vertices[k_v++] = (GLfloat) 0.f;
    for(size_t i = 0; i < (cfg.stacks - 1); ++i) {
        float phi = 180.f * (float) (i + 1) / (float) cfg.stacks;
        for(size_t j = 0; j < cfg.slices; ++j) {
            vertices[k_v++] = 360.f * (float) j / (float) cfg.slices;
            vertices[k_v++] = phi;
        }
    }
    vertices[k_v++] = (GLfloat) M_PI;
    vertices[k_v++] = (GLfloat) -M_PI;
    size_t index_count = cfg.slices * 6 * (cfg.stacks - 1);
    GLuint* indices = (GLuint*) malloc(index_count * sizeof(GLuint));
    if(indices == NULL) {
        LOG_ERROR("Unable to allocate globe index buffer.");
        free(vertices);
        return NULL;
    }
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
            indices[k_i++] = i3; indices[k_i++] = i0; 
            indices[k_i++] = i1;
            indices[k_i++] = i1; indices[k_i++] = i2; 
            indices[k_i++] = i3;
        }
    }
    Globe* mesh = (Globe*) malloc(sizeof(Globe));
    if(mesh == NULL) {
        LOG_ERROR("Unable to allocate Globe.");
        free(vertices);
        free(indices);
        return NULL;
    }
    mesh->vertices = vertices;
    mesh->indices = indices;
    mesh->vertex_count = vertex_count;
    mesh->index_count = index_count;
    return mesh;
}

void Globe_free(const Globe* const mesh) {
    free(mesh->vertices);
    free(mesh->indices);
    free((Globe*) mesh);
}

struct __GLOBE_H__GlobePass {
    size_t index_count;
    GLuint VAO, VBO, EBO, tex, shader_program;
};

const GlobePass* GlobePass_init(GlobePassDesc desc, const Globe* const mesh) {
    unsigned int failure;
    BitmapImage globe_texture;
    failure = BitmapImage_load_from_file(&globe_texture, desc.path_globe_texture);
    if(failure) return NULL;
    GLuint globe_texture_id;
    BitmapImage_build_texture(globe_texture, &globe_texture_id, GL_TEXTURE0);
    // configure earth shaders
    GLuint shader_program;
    failure = assemble_shader_program(&shader_program, desc.shader_vert, desc.shader_frag);
    if(failure) {
        BitmapImage_free(globe_texture);
        return NULL;
    };
    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    size_t buffer_size;
    buffer_size = mesh->vertex_count * 2 * sizeof(GLfloat);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr) buffer_size, mesh->vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    buffer_size = mesh->index_count * sizeof(GLuint);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr) buffer_size, mesh->indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 2, (GLvoid*) 0);
    glEnableVertexAttribArray(0);
    // pass the sampler for the earth texture
    glUseProgram(shader_program);
    glUniform1f(glGetUniformLocation(shader_program, "globe_radius"), desc.globe_radius);
    glUniform1f(glGetUniformLocation(shader_program, "globe_tex_offset"), desc.globe_tex_offset);
    glUniform1i(glGetUniformLocation(shader_program, "globe_tex_sampler"), 0);
    glUseProgram(0);
    BitmapImage_free(globe_texture);
    GlobePass* pass = (GlobePass*) malloc(sizeof(GlobePass));
    if(pass == NULL) {
        LOG_ERROR("Unable to allocate GlobePass.");
        glDeleteProgram(shader_program);
        glDeleteTextures(1, &globe_texture_id);
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
        return NULL;
    }
    pass->index_count = mesh->index_count;
    pass->VAO = VAO;
    pass->VBO = VBO;
    pass->EBO = EBO;
    pass->tex = globe_texture_id;
    pass->shader_program = shader_program;
    return pass;
}

void GlobePass_free(const GlobePass* const pass) {
    glDeleteProgram(pass->shader_program);
    glDeleteTextures(1, &(pass->tex));
    glDeleteVertexArrays(1, &(pass->VAO));
    glDeleteBuffers(1, &(pass->VBO));
    glDeleteBuffers(1, &(pass->EBO));
    free((GlobePass*) pass);
}

void GlobePass_update_and_draw(const GlobePass* const pass, const Camera* const cam) {
    Camera_update_uniforms(cam, pass->shader_program);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glUseProgram(pass->shader_program);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, pass->tex);
    glBindVertexArray(pass->VAO);
    size_t buffer_size;
    buffer_size = pass->index_count * sizeof(GLuint);
    glDrawElements(GL_TRIANGLES, (GLsizei) buffer_size, GL_UNSIGNED_INT, (GLvoid*) 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
    glActiveTexture(0);
    glUseProgram(0);
    glDisable(GL_DEPTH_TEST);
}
