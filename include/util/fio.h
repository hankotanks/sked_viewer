#ifndef FIO_H
#define FIO_H

#include <stdlib.h>
#include <stdio.h>

#include "log.h"

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

#endif /* FIO_H */