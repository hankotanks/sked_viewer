#ifndef SHADERS_H
#define SHADERS_H

#include <GL/glew.h>
#include <stdio.h>

#include "log.h"
#include "fio.h"

typedef enum {
    SHADER_LOC_PATH,
    SHADER_LOC_ID,
    SHADER_LOC_DESTROYED,
    SHADER_LOC_COUNT,
} ShaderLocator;

typedef struct {
    ShaderLocator loc;
    GLenum type;
    union { GLuint id; const char* path; } inner;
} Shader;

Shader Shader_init(const char* path, GLenum type) {
    Shader temp;
    temp.loc = SHADER_LOC_PATH;
    temp.type = type;
    temp.inner.path = path;
    return temp;
}

void Shader_destroy(Shader* shader) {
    switch(shader->loc) {
        case SHADER_LOC_ID:
            glDeleteShader(shader->inner.id);
            shader->loc = SHADER_LOC_DESTROYED;
            break;
        case SHADER_LOC_DESTROYED:
            LOG_INFO("Attempted to clean up an already-destroyed Shader.");
            break;
        case SHADER_LOC_PATH:
            LOG_INFO("Unable to clean up an uninitialized Shader.");
            break;
        case SHADER_LOC_COUNT:
        default: LOG_INFO("Attempted to clean up invalid Shader.");
    }
}

GLuint Shader_get_id(Shader* shader) {
    GLuint id;
    switch(shader->loc) {
        case SHADER_LOC_PATH:
            const char* source = read_file_contents(shader->inner.path);
            if(source == NULL || source[0] == '\0') {
                if(shader->type == GL_VERTEX_SHADER) {
                    LOG_ERROR("Unable to parse vertex shader from source.");
                } else if(shader->type == GL_FRAGMENT_SHADER) {
                    LOG_ERROR("Unable to parse fragment shader from source.");
                }
                return (GLuint) 0;
            }
            id = glCreateShader(shader->type);
            glShaderSource(id, 1, &source, NULL);
            free((char*) source);
            glCompileShader(id);
            GLint success;
            glGetShaderiv(id, GL_COMPILE_STATUS, &success);
            if(!success) {
                if(shader->type == GL_VERTEX_SHADER) {
                    LOG_ERROR("Failed to compile vertex shader.");
                } else if(shader->type == GL_FRAGMENT_SHADER) {
                    LOG_ERROR("Failed to compile fragment shader.");
                }
                GLint log_size;
                glGetShaderiv(id, GL_INFO_LOG_LENGTH, &log_size);
                char log[log_size];
                glGetShaderInfoLog(id, log_size, &log_size, log);
                printf("%s:\n%s\n", shader->inner.path, log);
                glDeleteShader(id);
                return (GLuint) 0;
            }
            shader->loc = SHADER_LOC_ID;
            shader->inner.id = id;
            break;
        case SHADER_LOC_ID:
            LOG_INFO("Shader has already been compiled. No work to be done.");
            id = shader->inner.id;
            break;
        case SHADER_LOC_DESTROYED:
            LOG_ERROR("Shader has been destroyed. Shader_get_id should not be called again.");
            return (GLuint) 0;
        case SHADER_LOC_COUNT: 
        default:
            LOG_ERROR("Shader has invalid locator value.");
            return (GLuint) 0;
    }
    return id;
}

unsigned int assemble_shader_program(GLuint* program, Shader* vert, Shader* frag) {
    GLuint vert_id, frag_id;
    *program = glCreateProgram();
    if(!(*program)) {
        LOG_ERROR("Failed to create program.");
        return 1;
    }
    unsigned int skipped = 0;
    if(vert == NULL) {
        LOG_INFO("Skipping vertex shader.");
        skipped = 1;
    } else {
        vert_id = Shader_get_id(vert);
        if(!vert_id) {
            LOG_ERROR("Compilation of vertex shader failed.");
            glDeleteProgram(*program);
            return 1;
        }
        glAttachShader(*program, vert_id);
    }
    if(frag == NULL) {
        if(skipped) {
            LOG_ERROR("Fragment shader cannot be empty after skipping vertex shader compilation.");
            glDeleteProgram(*program);
            return 1;
        } else {
            LOG_INFO("Skipping fragment shader.");
        }
    } else {
        frag_id = Shader_get_id(frag);
        if(!frag_id) {
            LOG_ERROR("Compilation of fragment shader failed.");
            glDeleteProgram(*program);
            return 1;
        }
        glAttachShader(*program, frag_id);
    }
    glLinkProgram(*program);
    GLint success;
    glGetProgramiv(*program, GL_LINK_STATUS, &success);
    if(!success) {
        LOG_ERROR("Shader program could not be linked.");
        glDeleteProgram(*program);
        return 1;
    }
    return 0;
}

#endif /* SHADERS_H */