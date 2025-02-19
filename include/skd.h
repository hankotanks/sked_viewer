#ifndef SKD_H
#define SKD_H

// TODO: This file needs a rewrite
// - consider which structs should be public
// - check which libraries are actually needed

#include <GL/glew.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "util/log.h"
#include "util/shaders.h"
#include "util/mjd.h"
#include "util/hashmap.h"

#define BUCKET_COUNT 10

typedef struct {
    char name[9];
    float lam, phi;
} Station;

void Station_debug(char* id, void* station);

typedef struct {
    char common_name[9];
    float alf, phi;
    uint8_t raan_hrs, raan_min; int8_t decl_deg, decl_min;
    float raan_sec, decl_sec;
} SourceQuasar;

void SourceQuasar_debug(char* iau, void* source);

typedef struct {
    Datetime timestamp;
    uint16_t cal_duration, obs_duration;
    char source[9];
    char ids[];
} Obs;

typedef struct {
    float color[3];
    Shader* shader_vert;
    Shader* shader_frag;
} SchedPassDesc;

typedef struct {
    HashMap stations_ant;
    HashMap stations_pos;
    // HashMap sources_sat;
    HashMap sources;
    HashMap sources_alias;
    size_t obs_count;
    Obs* obs;
} Schedule;

Obs* Schedule_get_observation(Schedule skd, size_t i);

unsigned int Schedule_debug_and_validate(Schedule skd, unsigned int display);

void Schedule_free(Schedule skd);

long seek_to_section(FILE* stream, const char* header);

void debug_station_antenna_keys(char* key, void* val) ;

unsigned int Schedule_build_from_source(Schedule* skd, const char* path);

#endif /* SKD_H */