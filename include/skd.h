#ifndef SKD_H
#define SKD_H

#include <GL/glew.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "./util/log.h"
#include "./util/shaders.h"
#include "./util/mjd.h"
#include "./util/hashmap.h"

#define BUCKET_COUNT 10

typedef struct {
    char name[9];
    float lam, phi;
} Station;

void Station_debug(char* id, void* station) {
    Station* temp = (Station*) station;
    printf("%s: %8s [%+7.2f, %+6.2f]\n", id, temp->name, temp->lam, temp->phi);
}

typedef struct {
    char common_name[9];
    float alf, phi;
    uint8_t raan_hrs, raan_min; int8_t decl_deg, decl_min;
    float raan_sec, decl_sec;
} SourceQuasar;

void SourceQuasar_debug(char* iau, void* source) {
    SourceQuasar* temp = (SourceQuasar*) source;
    if(temp->common_name[0] == '\0') {
        printf("%s: raan: [%2u:%2u:%+6.2f] decl: [%+3d:%+3d:%+6.2f]\n", iau, 
            temp->raan_hrs, temp->raan_min, temp->raan_sec, 
            temp->decl_deg, temp->decl_min, temp->decl_sec);
    } else {
        printf("%s (%s): raan: [%2u:%2u:%+6.2f] decl: [%+3d:%+3d:%+6.2f]\n", 
            iau, temp->common_name, 
            temp->raan_hrs, temp->raan_min, temp->raan_sec, 
            temp->decl_deg, temp->decl_min, temp->decl_sec);
    }
}

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

Obs* Schedule_get_observation(Schedule skd, size_t i) {
    if(i >= skd.obs_count) return NULL;
    return (Obs*) ((char*) skd.obs + i * (sizeof(Obs) + skd.stations_ant.size + 1));
}

unsigned int Schedule_debug_and_validate(Schedule skd, unsigned int display) {
    char station_key[2]; station_key[1] = '\0';
    char* station_id;
    char* quasar_id;
    Station* station_pos;
    SourceQuasar* quasar;
    Obs* curr;
    size_t i, j;
    for(i = 0; i < skd.obs_count; ++i) {
        curr = Schedule_get_observation(skd, i);
        // curr = (Obs*) ((char*) skd.obs + i * (sizeof(Obs) + skd.stations_ant.size + 1));
        if(display) printf("%8s [%s]: %4hu+%3hu [%2hhu:%2hhu:%2hhu]\n", 
            curr->source, curr->ids, 
            curr->timestamp.yrs, curr->timestamp.day, 
            curr->timestamp.hrs, curr->timestamp.min, curr->timestamp.sec);
        if((quasar_id = (char*) HashMap_get(skd.sources_alias, curr->source)) == NULL) quasar_id = curr->source;
        if((quasar = (SourceQuasar*) HashMap_get(skd.sources, quasar_id)) == NULL) {
            LOG_INFO("IAU source name is missing corresponding SourceQuasar entry in HashMap.");
            return 1;
        } else if(display) {
            printf("  ");
            SourceQuasar_debug(curr->source, (void*) quasar);
        }
        for(j = 0; j < strlen(curr->ids); ++j) {
            station_key[0] = curr->ids[j];
            if((station_id = (char*) HashMap_get(skd.stations_ant, station_key)) == NULL) {
                LOG_INFO("Antenna key in observation lacks matching HashMap entry.");
                return 1;
            } else {
                if((station_pos = (Station*) HashMap_get(skd.stations_pos, station_id)) == NULL) {
                    LOG_INFO("2-char station id is missing corresponding Station entry in HashMap.");
                    return 1;
                } else if(display) {
                    printf("  [%c] ", station_key[0]);
                    Station_debug(station_id, (void*) station_pos);
                }
            }
        }
    }
    if(display) printf("Successfully validated schedule.\n");
    return 0;
}

void Schedule_free(Schedule skd) {
    HashMap_free(skd.stations_ant);
    HashMap_free(skd.stations_pos);
    HashMap_free(skd.sources);
    HashMap_free(skd.sources_alias);
    free(skd.obs);
}

long seek_to_section(FILE* stream, const char* header) {
    ssize_t line_len;
    char* line = NULL;
    size_t line_cap = 0, header_len = strlen(header);
    while((line_len = getline(&line, &line_cap, stream)) != -1) {
        if((size_t) line_len >= header_len && strncmp(line, header, header_len) == 0) {
            free(line);
            return ftell(stream);
        }
    }
    free(line);
    return -1L;
}

void debug_station_antenna_keys(char* key, void* val) {
    printf("%s: %s\n", key, (char*) val);
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
        ret = sscanf(line, "P %2s %s %*f %*f %*f %*d %f %f %*s \n", id, station.name, &(station.lam), &(station.phi));
        if(ret == 4) {
            station.phi = 90.f - station.phi;
            HashMap_insert(&(skd->stations_pos), id, (void*) &station);
        } // TODO: If the station position fails to parse, the antenna entry should be removed
    }
    free(line);
    line = NULL;
    failure = fseek(stream, 0, SEEK_SET);
    CLOSE_STREAM_ON_FAILURE(stream, failure, 1, "Unable to seek to beginning of schedule.");
    sources_idx = seek_to_section(stream, "$SOURCES");
    CLOSE_STREAM_ON_FAILURE(stream, sources_idx < 0, 1, "Schedule contains no $SOURCES section.");
    HashMap_init(&(skd->sources), BUCKET_COUNT, sizeof(SourceQuasar));
    HashMap_init(&(skd->sources_alias), BUCKET_COUNT, 9);
    char iau[9];
    SourceQuasar source;
    while((line_len = getline(&line, &line_cap, stream)) != -1) {
        if(line[0] == '$') break;
        ret = sscanf(line, " %8s %8s %hhu %hhu %f %hhd %hhd %f %*f %*f %*s %*s \n",
            iau, source.common_name,
            &(source.raan_hrs), &(source.raan_min), &(source.raan_sec), 
            &(source.decl_deg), &(source.decl_min), &(source.decl_sec));
        if(ret == 8) {
            source.alf = (float) source.raan_hrs + (float) source.raan_min / 60.f + source.raan_sec / 3600.f;
            source.alf *= 15.f;
            source.phi = (float) source.decl_deg + (float) source.decl_min / 60.f + source.decl_sec / 3600.f;
            source.phi = 90.f - source.phi; // TODO: Since all sked data has this 90 deg. offset, maybe it should be baked into a function
            if(source.common_name[0] == '$') source.common_name[0] = '\0';
            else HashMap_insert(&(skd->sources_alias), source.common_name, iau);
            HashMap_insert(&(skd->sources), iau, &source);
        }
    }
    free(line);
    line = NULL;
    obs_idx = seek_to_section(stream, "$SKED");
    CLOSE_STREAM_ON_FAILURE(stream, obs_idx < 0, 1, "Schedule contains no $SKED section.");
    skd->obs_count = 0;
    while((line_len = getline(&line, &line_cap, stream)) != -1) {
        if(line[0] == '$') break;
        (skd->obs_count)++;
    }
    free(line);
    line = NULL;
    skd->obs = (Obs*) malloc(skd->obs_count * (sizeof(Obs) + skd->stations_ant.size + 1));
    failure = fseek(stream, 0, SEEK_SET);
    CLOSE_STREAM_ON_FAILURE(stream, failure, 1, "Unable to seek to beginning of schedule.");
    obs_idx = seek_to_section(stream, "$SKED");
    CLOSE_STREAM_ON_FAILURE(stream, obs_idx < 0, 1, "Failed to return to Schedule's $SKED section.");
    char timestamp_raw[12];
    char cable_wrap[skd->stations_ant.size * 2 + 1];
    Obs* current;
    size_t i = 0, j;
    while((line_len = getline(&line, &line_cap, stream)) != -1 && i < skd->obs_count) {
        if(line[0] == '$') break;
        current = Schedule_get_observation(*skd, i);
        // current = (Obs*) ((char*) skd->obs + i * (sizeof(Obs) + skd->stations_ant.size + 1));
        ret = sscanf(line, " %8s %hu %*c%*c %*s %s %hu %*s %*u %*s %s %*s \n",
            current->source, &(current->cal_duration), timestamp_raw, &(current->obs_duration), cable_wrap);
        if(ret == 5) {
            failure = Datetime_parse_from_obs(&(current->timestamp), "y2d3h2m2s2", timestamp_raw);
            if(failure) {
                LOG_INFO("Failed to parse observation Datetime. Skipping.");
                continue;
            }
            if(strlen(cable_wrap) % 2 != 0) {
                LOG_INFO("Invalid cable wrap string. Skipping observation.");
                continue;
            }
            for(j = 0; j < strlen(cable_wrap) / 2; ++j) current->ids[j] = cable_wrap[j * 2];
            current->ids[j + 1] = '\0';
            if(current->timestamp.yrs < 100) {
                if(current->timestamp.yrs > 78) current->timestamp.yrs += 1900; // TODO: Look for a better way to handle this
                else current->timestamp.yrs += 2000;
            }
            i++;
        } else {
            LOG_INFO("Failed to parse observation.");
        }
    }
    free(line);
    skd->obs_count = i;
    fclose(stream);
    return 0;
}

#endif /* SKD_H */