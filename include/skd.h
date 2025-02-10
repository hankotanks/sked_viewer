#ifndef SKD_H
#define SKD_H

#include <GL/glew.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "./util/log.h"
#include "./util/mjd.h"
#include "./util/hashmap.h"

#define BUCKET_COUNT 10

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

void Station_debug(char* id, void* station) {
    Station* temp = (Station*) station;
    printf("%s: %8s [%+7.2f, %+6.2f]\n", id, temp->name, temp->lat, temp->lon);
}

typedef struct {
    char common_name[9];
    uint8_t raan_hh, raan_mm;
    int8_t decl_hh, decl_mm;
    float raan_ss;
    float decl_ss;
    float epoch;
} SourceQuasar;

void SourceQuasar_debug(char* iau, void* source) {
    SourceQuasar* temp = (SourceQuasar*) source;
    if(temp->common_name[0] == '\0') {
        printf("%s: raan: [%2u:%2u:%+6.2f] decl: [%+3d:%+3d:%+6.2f] epoch: %.1f\n", iau, 
            temp->raan_hh, temp->raan_mm, temp->raan_ss, 
            temp->decl_hh, temp->decl_mm, temp->decl_ss, temp->epoch);
    } else {
        printf("%s (%s): raan: [%2u:%2u:%+6.2f] decl: [%+3d:%+3d:%+6.2f] epoch: %.1f\n", 
            iau, temp->common_name, 
            temp->raan_hh, temp->raan_mm, temp->raan_ss, 
            temp->decl_hh, temp->decl_mm, temp->decl_ss, temp->epoch);
    }
}

typedef struct {
    char source[9];
    Datetime timestamp;
    uint16_t cal_duration, obs_duration;
    char ids[];
} Obs;

typedef struct {
    HashMap stations_ant;
    HashMap stations_pos;
    // HashMap sources_sat;
    HashMap sources;
    size_t obs_count;
    Obs* obs;
} Schedule;

void Schedule_free(Schedule skd) {
    HashMap_free(skd.stations_ant);
    HashMap_free(skd.stations_pos);
    HashMap_free(skd.sources);
}

long seek_to_section(FILE* stream, const char* header) {
    unsigned int found;
    long index;
    ssize_t line_len;
    char* line = NULL;
    size_t line_cap = 0, header_len = strlen(header);
    while((line_len = getline(&line, &line_cap, stream)) != -1) {
        if(found) return ftell(stream);
        if(line_len >= header_len && strncmp(line, header, header_len) == 0) found = 1;
    }
    free(line);
    return -1L;
}

void debug_stations_ant_hashmap(char* id, void* val) {
    printf("%s: %s\n", id, (char*) val);
}

unsigned int Schedule_build_from_source(Schedule* skd, const char* path) {
    FILE* stream = fopen(path, "rb");
    if(stream == NULL) {
        LOG_ERROR("Schedule couldn't be opened.");
        return 1;
    }
    unsigned int failure;
    long stations_idx, sources_idx, obs_idx;
    stations_idx = seek_to_section(stream, "$STATIONS");
    CLOSE_STREAM_ON_FAILURE(stream, stations_idx < 0, 1, "Schedule contains no $STATIONS section.");
    HashMap_init(&(skd->stations_ant), BUCKET_COUNT, 3);
    HashMap_init(&(skd->stations_pos), 10, sizeof(Station));
    ssize_t line_len; size_t line_cap = 0;
    char key[2], id[3]; 
    char* line = NULL;
    Station station;
    int ret;
    while((line_len = getline(&line, &line_cap, stream)) != -1) {
        // TODO: Why does the following line break the parsing
        // if(line[0] == '$') break;
        ret = sscanf(line, "A %1s %*s %*s %*f %*f %*d %*f %*f %*f %*d %*f %*f %*f %2s %*s %*s \n", key, id);
        if(ret == 2) HashMap_insert(&(skd->stations_ant), key, (void*) id);
        ret = sscanf(line, "P %2s %s %*f %*f %*f %*d %f %f %*s \n", id, station.name, &(station.lat), &(station.lon));
        if(ret == 4) HashMap_insert(&(skd->stations_pos), id, (void*) &station);
    }
    failure = fseek(stream, 0, SEEK_SET);
    CLOSE_STREAM_ON_FAILURE(stream, failure, 1, "Unable to seek to beginning of schedule.");
    sources_idx = seek_to_section(stream, "$SOURCES");
    CLOSE_STREAM_ON_FAILURE(stream, sources_idx < 0, 1, "Schedule contains no $SOURCES section.");
    HashMap_init(&(skd->sources), BUCKET_COUNT, sizeof(SourceQuasar));
    char iau[9];
    SourceQuasar source;
    while((line_len = getline(&line, &line_cap, stream)) != -1) {
        if(line[0] == '$') break;
        ret = sscanf(line, " %8s %8s %hhu %hhu %f %hhd %hhd %f %f %*f %*s %*s \n",
            iau, source.common_name,
            &(source.raan_hh), &(source.raan_mm), &(source.raan_ss), 
            &(source.decl_hh), &(source.decl_mm), &(source.decl_ss), &(source.epoch));
        if(ret == 9) {
            if(source.common_name[0] == '$') source.common_name[0] = '\0';
            HashMap_insert(&(skd->sources), iau, &source);
        }
    }
    obs_idx = seek_to_section(stream, "$SKED");
    CLOSE_STREAM_ON_FAILURE(stream, obs_idx < 0, 1, "Schedule contains no $SKED section.");
    skd->obs_count = 0;
    while((line_len = getline(&line, &line_cap, stream)) != -1) {
        if(line[0] == '$') break;
        (skd->obs_count)++;
    }
    skd->obs = (Obs*) malloc(skd->obs_count * sizeof(Obs));
    obs_idx = seek_to_section(stream, "$SKED");
    CLOSE_STREAM_ON_FAILURE(stream, obs_idx < 0, 1, "Failed to return to Schedule's $SKED section.");
    // TODO: From here on is in-progress
    char start_timestamp[12];
    char cable_wrap[skd->stations_ant.size * 2 + 1];
    size_t i = 0;
    while((line_len = getline(&line, &line_cap, stream)) != -1 && i < skd->obs_count) {
        if(line[0] == '$') break;
// 0308-611  10 SX PREOB  25030190906        80 MIDOB         0 POSTOB F-JCLCYW 1F000000 1F000000 1F000000 1F000000 YYNN    76    80    70    80 
        ret = sscanf(line, " %8s %hu",//%*c%*c %*s %s %u %*s %*u %*s %s %*s \n",
            skd->obs[i].source, &(skd->obs[i].cal_duration));//, start_timestamp, &(skd->obs[i].obs_duration), cable_wrap);
        if(ret != 0) printf("%s\n", skd->obs[i].source);
        i++;
    }
    // //
    /*
    char source[9];
    Datetime timestamp;
    uint16_t cal_duration, obs_duration;
    char ids[];
    // //
    char cable_wrap[skd->stations_ant.size + 1];
    Obs obs;
    while((line_len = getline(&line, &line_cap, stream)) != -1) {
        if(line[0] == '$') break;
        ret = sscanf(line, " %8s %hu %*c%*c %*s %2hhu%3hu%2hhu%2hhu%2hhu %u %*s %*u %*s %s %*s \n",
            obs.source, &(obs.cal_duration), )
    }*/
    fclose(stream);
    //HashMap_dump(skd->stations_pos, Station_debug);
    //HashMap_dump(skd->stations_ant, debug_stations_ant_hashmap);
    // HashMap_dump(sources, Source_debug);
}

#endif /* SKD_H */