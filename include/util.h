#ifndef UTIL_H
#define UTIL_H

#include <GL/glew.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

#ifdef LOGGING
#define LOG_ERROR(msg) fprintf(stderr, "ERROR: %s\n", msg);
#define LOG_INFO(msg) printf("INFO: %s\n", msg);
#define CLOSE_STREAM_ON_FAILURE(stream, failure, val, msg) if(failure) {\
    fclose(stream);\
    LOG_ERROR(msg);\
    return val;\
}
#else
#define LOG_ERROR(msg)
#define LOG_INFO(msg)
#define CLOSE_STREAM_ON_FAILURE(stream, failure, val, msg) if(failure) {\
    fclose(stream);\
    return val;\
}
#endif

// Adapted from https://stackoverflow.com/a/11793817
// Credit to users Senna and Paul Rooney
const char* read_file_contents(const char* path) {
    FILE* stream;
    char* contents;
    stream = fopen(path, "rb");
    if(stream == NULL) {
        LOG_ERROR("Stream couldn't be opened.");
        return NULL;
    }
    int failure;
    failure = fseek(stream, 0L, SEEK_END);
    CLOSE_STREAM_ON_FAILURE(stream, failure, NULL, "Failed to seek to end of file.");
    long file_size_temp = ftell(stream);
    CLOSE_STREAM_ON_FAILURE(stream, file_size_temp < 0, NULL, "File size is invalid.");
    size_t file_size = (size_t) file_size_temp;
    failure = fseek(stream, 0L, SEEK_SET);
    CLOSE_STREAM_ON_FAILURE(stream, failure, NULL, "Failed to seek to beginning of file.");
    contents = (char*) malloc(file_size + 1);
    CLOSE_STREAM_ON_FAILURE(stream, contents == NULL, NULL, "Unable to allocate char buffer for file contents.");
    size_t size = fread(contents,1,file_size, stream);
    failure = size < file_size && !feof(stream);
    CLOSE_STREAM_ON_FAILURE(stream, failure, NULL, "Read less bytes than expected.");
    contents[size] = 0;
    fclose(stream);
    return contents;
}

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

typedef struct {
    unsigned char header[54];
    size_t data_offset;
    size_t data_size;
    size_t w, h;
    unsigned char* data;
} BitmapImage;

void BitmapImage_free(BitmapImage img) {
    free(img.data);
}

void BitmapImage_build_texture(BitmapImage img, GLuint* tex_id, GLenum tex_unit) {
    glActiveTexture(tex_unit);
    glGenTextures(1, tex_id);
    glBindTexture(GL_TEXTURE_2D, *tex_id);
    GLsizei w = (GLsizei) img.w;
    GLsizei h = (GLsizei) img.h;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_BGR, GL_UNSIGNED_BYTE, img.data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glActiveTexture(0);
}

// Sourced BMP parser from https://www.opengl-tutorial.org/beginners-tutorials/tutorial-5-a-textured-cube/
int BitmapImage_load_from_file(BitmapImage* img, const char* path) {
    FILE* stream = fopen(path, "rb");
    CLOSE_STREAM_ON_FAILURE(stream, stream == NULL, 1, "Failed to open image file.");
    size_t header_size = fread(img->header, 1, 54, stream);
    CLOSE_STREAM_ON_FAILURE(stream, header_size != 54, 1, "Image header was incorrectly sized.");
    CLOSE_STREAM_ON_FAILURE(stream, img->header[0] != 'B' || img->header[1] != 'M', 1, "Image header must begin with `BM`.");
    img->data_offset = (size_t) (*(int*) &(img->header[0x0A]));
    img->data_size   = (size_t) (*(int*) &(img->header[0x22]));
    img->w = (size_t) (*(int*) &(img->header[0x12]));
    img->h = (size_t) (*(int*) &(img->header[0x16]));
    if(img->data_size == 0) {
        LOG_INFO("Header does not report size of data. Estimating image size instead.");
        img->data_size = img->w * img->h * 3;
    }
    if(img->data_offset == 0) {
        LOG_INFO("Image data offset not indicated by header. Using default byte offset of 54.");
        img->data_offset = 54;
    }
    img->data = (unsigned char*) malloc(img->data_size);
    CLOSE_STREAM_ON_FAILURE(stream, img->data == NULL, 1, "Unable to allocate buffer for pixel data.");
    size_t read_size = fread(img->data, 1, img->data_size, stream);
    CLOSE_STREAM_ON_FAILURE(stream, read_size < img->data_size, 1, "Mismatch in size between image buffer and image data.");
    fclose(stream);
    return 0;
}

#endif /* UTIL_H */