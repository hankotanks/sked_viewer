#ifndef UTIL_H
#define UTIL_H

#include <stdlib.h>
#include <stdio.h>

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

#endif /* UTIL_H */