#ifndef SHADERS_H
#define SHADERS_H

#include <GL/glew.h>

GLuint compile_shader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if(!success) return (GLuint) 0;
    return shader;
}

GLuint compile_shader_program(const char* vert, const char* frag) {
    GLuint program = glCreateProgram();
    GLuint vert_shader = compile_shader(GL_VERTEX_SHADER, vert);
    if(vert[0] != '\0') glAttachShader(program, vert_shader);
    GLuint frag_shader = compile_shader(GL_FRAGMENT_SHADER, frag);
    if(frag[0] != '\0') glAttachShader(program, frag_shader);
    glLinkProgram(program);
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    glDeleteShader(vert_shader);
    glDeleteShader(frag_shader);
    if(!success) return (GLuint) 0;
    return program;
}

#endif /* SHADERS_H */