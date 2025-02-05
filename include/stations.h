#ifndef STATIONS_H
#define STATIONS_H

#include <stdint.h>
#include <stdio.h>

#include "util.h"

typedef struct {
    char (*ids)[2];
    GLfloat* pos;
    size_t station_count;
} Catalog;

void Catalog_free(Catalog cat) {
    free(cat.ids);
    free(cat.pos);
}

void Catalog_configure_buffers(Catalog cat, GLuint* VAO, GLuint* VBO) {
    glGenVertexArrays(1, VAO);
    glGenBuffers(1, VBO);
    glBindVertexArray(*VAO);
    glBindBuffer(GL_ARRAY_BUFFER, *VBO);
    glBufferData(GL_ARRAY_BUFFER, cat.station_count * 3 * sizeof(GLfloat), cat.pos, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3, (GLvoid*) 0);
    glEnableVertexAttribArray(0);
}

void Catalog_draw(Catalog cat, GLuint VAO) {
    glBindVertexArray(VAO);    
    glDrawArrays(GL_POINTS, 0, (GLsizei) cat.station_count);
}

Catalog Catalog_parse_from_file(const char* raw) {
    Catalog cat;
    cat.station_count = 0;
    FILE* stream;
    stream = fopen(raw, "rb");
    if(stream == NULL) {
        LOG_ERROR("Station position catalog couldn't be opened.");
        return cat;
    }
    int failure;
    failure = fseek(stream, 0L, SEEK_END);
    CLOSE_STREAM_ON_FAILURE(stream, failure, cat, "Failed to seek to end of file.");
    long file_size = ftell(stream);
    CLOSE_STREAM_ON_FAILURE(stream, file_size < 0, cat, "File size is invalid.");
    failure = fseek(stream, 0L, SEEK_SET);
    CLOSE_STREAM_ON_FAILURE(stream, failure, cat, "Failed to seek to beginning of file.");
    ssize_t flag;
    char* line = NULL;
    size_t len = 0;
    size_t station_count = 0;
    while((flag = getline(&line, &len, stream)) != -1) {
        unsigned int comment = 1;
        for(size_t i = 0; i < (size_t) flag; ++i) {
            switch(line[i]) {
                case ' ':
                case '\t': break;
                case '*':
                case '\r':
                case '\n': goto comment_check_end;
                default:
                    comment = 0;
                    goto comment_check_end;
            }
        } 
comment_check_end:
        if(!comment) station_count++;
        if(file_size - ftell(stream) == 0) {
            flag = 0;
            break;
        }
    }
    CLOSE_STREAM_ON_FAILURE(stream, flag == -1, cat, "Failed to count stations before parsing.");
    failure = fseek(stream, 0L, SEEK_SET);
    CLOSE_STREAM_ON_FAILURE(stream, failure, cat, "Failed to seek to beginning of catalog.");
    cat.ids = (char (*)[2]) malloc(station_count * sizeof(*(cat.ids)));
    CLOSE_STREAM_ON_FAILURE(stream, cat.ids == NULL, cat, "Unable to allocate memory for station ids.");
    cat.pos = (GLfloat*) malloc(station_count * 3 * sizeof(GLfloat));
    if(cat.pos == NULL) {
        free(cat.ids);
        cat.ids = NULL;
        CLOSE_STREAM_ON_FAILURE(stream, 1, cat, "Unable to allocate memory for station coords.");
    }
    size_t idx = 0;
    while((flag = getline(&line, &len, stream)) != -1) {
        unsigned int comment = 1;
        size_t i;
        for(i = 0; i < len; ++i) {
            switch(line[i]) {
                case ' ':
                case '\t': break;
                case '*':
                case '\r':
                case '\n': goto station_parse_start;
                default:
                    comment = 0;
                    goto station_parse_start;
            }
        } 
station_parse_start:
        if(comment && file_size > ftell(stream)) continue;
        char id[3];
        float x, y, z;
        int ret = sscanf(line, "%2s %*s %f %f %f %*d %*f %*f %*s", id, &x, &y, &z);
        if(ret != 4) {
            free(cat.ids); cat.ids = NULL;
            free(cat.pos); cat.pos = NULL;
            CLOSE_STREAM_ON_FAILURE(stream, 1, cat, "Position catalog has invalid entries.");
        }
        cat.ids[idx][0] = id[0];
        cat.ids[idx][1] = id[1];
        cat.pos[idx * 3 + 0] = (GLfloat) x / 1000.f * -1.f;
        cat.pos[idx * 3 + 1] = (GLfloat) z / 1000.f;
        cat.pos[idx * 3 + 2] = (GLfloat) y / 1000.f;
        idx++;
        if(file_size - ftell(stream) == 0) {
            flag = 0;
            break;
        }
    }
    CLOSE_STREAM_ON_FAILURE(stream, flag == -1, cat, "Failed to parse stations.");
    cat.station_count = station_count;
    return cat;
}

#endif /* STATIONS_H */