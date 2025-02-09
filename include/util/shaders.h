#ifndef SHADERS_H
#define SHADERS_H

#include <GL/glew.h>

#include "log.h"
#include "fio.h"

unsigned int compile_shader_from_source(GLuint* shader, GLenum type, const char* source) {
    *shader = glCreateShader(type);
    glShaderSource(*shader, 1, &source, NULL);
    glCompileShader(*shader);
    GLint success;
    glGetShaderiv(*shader, GL_COMPILE_STATUS, &success);
    if(!success) {
        if(type == GL_VERTEX_SHADER) LOG_ERROR("Failed to compile vertex shader.");
        if(type == GL_FRAGMENT_SHADER) LOG_ERROR("Failed to compile fragment shader.");
        glDeleteShader(*shader);
        return 1;
    }
    return 0;
}

unsigned int compile_shader(GLuint* shader, GLenum type, const char* path) {
    const char* source = read_file_contents(path);
    if(source == NULL || source[0] == '\0') {
        LOG_ERROR("Failed to read shader source.");
        return 1;
    }
    *shader = glCreateShader(type);
    glShaderSource(*shader, 1, &source, NULL);
    glCompileShader(*shader);
    GLint success;
    glGetShaderiv(*shader, GL_COMPILE_STATUS, &success);
    free((char*) source);
    if(!success) {
        if(type == GL_VERTEX_SHADER) LOG_ERROR("Failed to compile vertex shader.");
        if(type == GL_FRAGMENT_SHADER) LOG_ERROR("Failed to compile fragment shader.");
        glDeleteShader(*shader);
        return 1;
    }
    return 0;
}

unsigned int assemble_shader_program(GLuint* program, GLuint vert, GLuint frag) {
    *program = glCreateProgram();
    if(!(*program)) {
        LOG_ERROR("Failed to create program.");
        return 1;
    }
    unsigned int skipped = 0;
    if(vert == (GLuint) 0) {
        LOG_INFO("Skipping vertex shader.");
        skipped = 1;
    } else glAttachShader(*program, vert);
    if(frag == (GLuint) 0) {
        if(skipped) {
            LOG_ERROR("Fragment shader cannot be empty after skipping vertex shader compilation.");
            glDeleteProgram(*program);
            return 1;
        } else LOG_INFO("Skipping fragment shader.");
    } else glAttachShader(*program, frag);
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