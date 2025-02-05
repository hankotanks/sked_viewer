#ifndef STATIONS_H
#define STATIONS_H

#include <stdint.h>
#include <stdio.h>

#include "util.h"

typedef struct {
    char (*ids)[2];
    GLfloat* lam;
    GLfloat* phi;
    size_t station_count;
} Catalog;

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
        for(size_t i = 0; i < flag; ++i) {
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
    cat.lam = (GLfloat*) malloc(station_count * sizeof(GLfloat));
    if(cat.lam == NULL) free(cat.ids);
    CLOSE_STREAM_ON_FAILURE(stream, cat.lam == NULL, cat, "Unable to allocate memory for station coords.");
    cat.phi = (GLfloat*) malloc(station_count * sizeof(GLfloat));
    if(cat.phi == NULL) {
        free(cat.ids);
        free(cat.lam);
    }
    CLOSE_STREAM_ON_FAILURE(stream, cat.phi == NULL, cat, "Unable to allocate memory for station coords.");
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
        float lam, phi;
        int ret = sscanf(line, "%2s %*s %*f %*f %*f %*d %f %f %*s", id, &lam, &phi);
        cat.ids[idx][0] = id[0];
        cat.ids[idx][1] = id[1];
        cat.lam[idx] = (GLfloat) lam;
        cat.phi[idx] = (GLfloat) phi;
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