#ifndef UTIL_H
#define UTIL_H

#include <GL/glew.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

// Adapted from https://stackoverflow.com/a/11793817
// Credit to user Senna and Paul Rooney
const char* read_file_contents(const char* path) {
    int success;
    FILE* stream;
    char* contents;
    long file_size = 0;
    stream = fopen(path, "rb");
    if(stream == NULL) return NULL;
    success = fseek(stream, 0L, SEEK_END);
    if(success) {
        fclose(stream);
        return NULL;
    }
    file_size = ftell(stream);
    if(file_size == -1) {
        fclose(stream);
        return NULL;
    }
    success = fseek(stream, 0L, SEEK_SET);
    if(success) {
        fclose(stream);
        return NULL;
    }
    contents = (char*) malloc(file_size + 1);
    if(contents == NULL) {
        fclose(stream);
        return NULL;
    }
    size_t size = fread(contents,1,file_size, stream);
    if(size < file_size && !feof(stream)) {
        fclose(stream);
        return NULL;
    }
    contents[size] = 0;
    fclose(stream);
    return contents;
}

GLuint compile_shader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if(!success) {
        glDeleteShader(shader);
        return (GLuint) 0;
    }
    return shader;
}

GLuint compile_shader_program(const char* vert, const char* frag) {
    GLuint program = glCreateProgram();
    if(!program) return (GLuint) 0;
    unsigned int skipped = 0;
    GLuint vert_shader, frag_shader;
    if(vert[0] != '\0') {
        vert_shader = compile_shader(GL_VERTEX_SHADER, vert);
        if(!vert_shader) return (GLuint) 0;
        glAttachShader(program, vert_shader);
    } else skipped = 1;
    if(frag[0] == '\0' && skipped) return (GLuint) 0;
    else {
        frag_shader = compile_shader(GL_FRAGMENT_SHADER, frag);
        if(!frag_shader) {
            glDeleteShader(vert_shader);
            return (GLuint) 0;
        }
        glAttachShader(program, frag_shader);
    }
    glLinkProgram(program);
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    glDeleteShader(vert_shader);
    glDeleteShader(frag_shader);
    if(!success) return (GLuint) 0;
    return program;
}

// TODO: Move image functions to their own header?
typedef struct {
    unsigned char header[54];
    size_t data_offset;
    size_t data_size;
    size_t w, h;
    unsigned char* data;
} BitmapImage;

void BitmapImage_build_texture(BitmapImage img, GLuint* id) {
    glGenTextures(1, id);
    glBindTexture(GL_TEXTURE_2D, *id);
    GLsizei w = (GLsizei) img.w;
    GLsizei h = (GLsizei) img.h;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_BGR, GL_UNSIGNED_BYTE, img.data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
}

// Sourced BMP parser from https://www.opengl-tutorial.org/beginners-tutorials/tutorial-5-a-textured-cube/
int BitmapImage_load_from_file(BitmapImage* img, const char* path) {
    FILE* stream = fopen(path, "rb");
    if(stream == NULL) return 1;
    if(fread(img->header, 1, 54, stream) != 54) return 2;
    if(img->header[0] != 'B' || img->header[1] != 'M') return 3;
    img->data_offset = (size_t) (*(int*) &(img->header[0x0A]));
    img->data_size   = (size_t) (*(int*) &(img->header[0x22]));
    img->w = (size_t) (*(int*) &(img->header[0x12]));
    img->h = (size_t) (*(int*) &(img->header[0x16]));
    if(img->data_size == 0) img->data_size = img->w * img->h * 3;
    if(img->data_offset == 0) img->data_offset = 54;
    img->data = (unsigned char*) malloc(img->data_size);
    if(img->data == NULL) return 4;
    fread(img->data, 1, img->data_size, stream); // TODO: Guard?
    fclose(stream);
    return 0;
}



#endif /* UTIL_H */