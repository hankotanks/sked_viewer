#ifndef SKD_H
#define SKD_H

#include <GL/glew.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "./util/log.h"
#include "./util/hashmap.h"

/*
typedef struct {
    size_t station_count;
    struct {
        char id[2];
        char id_antenna;
        char name[8];
        float lat, lon;
    }* catalog;
    size_t source_count;
    struct {
        char iau[7];
        char name[8];
        uint8_t alph_hrs, alph_min, decl_hrs, decl_min;
        float alph_sec, decl_sec, epoch;
    }* sources;
    size_t obs_count;
    struct {
        uint32_t start;
        uint32_t duration;
        size_t source;
        size_t stations[];
    } obs;
} Schedule;
*/

typedef struct {
    char name[9];
    float lat, lon;
} Station;

long seek_to_section(FILE* stream, const char* header) {
    unsigned int found;
    long index;
    ssize_t line_len;
    char* line = NULL;
    size_t line_cap = 0, header_len = strlen(header);
    while((line_len = getline(&line, &line_cap, stream)) != -1) {
        //printf("%s\n", line);
        if(found) return ftell(stream);
        if(line_len >= header_len && strncmp(line, header, header_len) == 0) found = 1;
    }
    free(line);
    return -1L;
}

void Station_debug(char* id, void* station) {
    Station* temp = (Station*) station;
    printf("%s: %s at [%f, %f]\n", id, temp->name, temp->lat, temp->lon);
}

void debug_antenna_entry(char* key, void* id) {
    printf("%s: %s\n", key, (char*) id);
}

unsigned int parse_schedule_from_source(const char* path) {
    FILE* stream = fopen(path, "rb");
    if(stream == NULL) {
        LOG_ERROR("Schedule couldn't be opened.");
        return 1;
    }
    unsigned int failure;
    long station_section_start, source_section_start;
    station_section_start = seek_to_section(stream, "$STATIONS");
    CLOSE_STREAM_ON_FAILURE(stream, station_section_start < 0, 1, "Schedule contained no $STATIONS section.");
    HashMap station_antennas, station_positions;
    HashMap_init(&station_antennas, 10, 3);
    HashMap_init(&station_positions, 10, sizeof(Station));
    ssize_t line_len; size_t line_cap = 0;
    char* key[2], id[3], line = NULL;
    Station station;
    while((line_len = getline(&line, &line_cap, stream)) != -1) {
        int ret = sscanf(line, "A %1s %*s %*s %*f %*f %*d %*f %*f %*f %*d %*f %*f %*f %2s %*s %*s\n", key, id);
        if(ret == 2) HashMap_insert(&station_antennas, key, (void*) id);
        ret = sscanf(line, "P %2s %s %*f %*f %*f %*d %f %f %*s\n", id, station.name, &(station.lat), &(station.lon));
        if(ret == 4) HashMap_insert(&station_positions, id, (void*) &station);
    }
    failure = fseek(stream, 0, SEEK_SET);
    CLOSE_STREAM_ON_FAILURE(stream, failure, 1, "Unable to seek to beginning of schedule.");
    source_section_start = seek_to_section(stream, "$SOURCES");
    CLOSE_STREAM_ON_FAILURE(stream, source_section_start < 0, 1, "Schedule contained no $SOURCES section.");
    // PARSE SOURCES HERE
    fclose(stream);
    HashMap_dump(station_antennas, debug_antenna_entry);
    HashMap_dump(station_positions, Station_debug);
    HashMap_free(station_antennas);
    HashMap_free(station_positions);
}

#endif /* SKD_H */