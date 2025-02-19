#ifndef BMP_H
#define BMP_H

#include <GL/glew.h>
#include <stdlib.h>
#include <stdio.h>
#include "log.h"

typedef struct {
    unsigned char header[54];
    size_t data_offset;
    size_t data_size;
    size_t w, h;
    unsigned char* data;
} BitmapImage;

static void BitmapImage_free(BitmapImage img) {
    free(img.data);
}

static void BitmapImage_build_texture(BitmapImage img, GLuint* tex_id, GLenum tex_unit) {
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
static int BitmapImage_load_from_file(BitmapImage* img, const char* path) {
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

#endif /* BMP_H */