#include "skd.h"
#include <stddef.h>
#include <stdio.h>
#include "util/mjd.h"
#include "util/hashmap.h"

unsigned int Schedule_debug_and_validate(Schedule skd, unsigned int display) {
    char station_key[2]; station_key[1] = '\0';
    char* station_id;
    char* quasar_id;
    Station* station_pos;
    SourceQuasar* quasar;
    Scan* curr;
    size_t i, j;
    for(i = 0; i < skd.scan_count; ++i) {
        curr = Schedule_get_scan(skd, i);
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
            if(quasar->name[0] == '\0') {
                printf("%s [%+8.2f, %+8.2f]\n", 
                    curr->source, quasar->alf, quasar->phi);
            } else {
                printf("%s (%s) [%+8.2f, %+8.2f]\n", 
                    curr->source, quasar->name, quasar->alf, quasar->phi);
            }
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
                    printf("%s: %8s [%+7.2f, %+6.2f]\n", station_id, 
                        station_pos->name, station_pos->lam, station_pos->phi);
                }
            }
        }
    }
    if(display) printf("Successfully validated schedule.\n");
    return 0;
}

Scan* Schedule_get_scan(Schedule skd, size_t i) {
    if(i >= skd.scan_count) return NULL;
    return (Scan*) ((char*) skd.scans + i * (sizeof(Scan) + skd.stations_ant.size + 1));
}

void Schedule_free(Schedule skd) {
    HashMap_free(skd.stations_ant);
    HashMap_free(skd.stations_pos);
    HashMap_free(skd.sources);
    HashMap_free(skd.sources_alias);
    free(skd.scans);
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

#define BUCKET_COUNT 10 // TODO: Allow this to be configured
unsigned int Schedule_build_from_source(Schedule* skd, const char* path) {
    FILE* stream = fopen(path, "rb");
    if(stream == NULL) {
        LOG_ERROR("Schedule couldn't be opened.");
        return 1;
    }
    unsigned int failure;
    long stations_idx, sources_idx, scan_idx;
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
    uint8_t raan_hrs, raan_min; int8_t decl_deg, decl_min;
    float raan_sec, decl_sec;
    while((line_len = getline(&line, &line_cap, stream)) != -1) {
        if(line[0] == '$') break;
        ret = sscanf(line, " %8s %8s %hhu %hhu %f %hhd %hhd %f %*f %*f %*s %*s \n",
            iau, source.name,
            &raan_hrs, &raan_min, &raan_sec, 
            &decl_deg, &decl_min, &decl_sec);
        if(ret == 8) {
            source.alf = (float) raan_hrs + (float) raan_min / 60.f + raan_sec / 3600.f;
            source.alf *= 15.f;
            source.phi = (float) decl_deg + (float) decl_min / 60.f + decl_sec / 3600.f;
            source.phi = 90.f - source.phi; // TODO: Since all sked data has this 90 deg. offset, maybe it should be baked into a function
            if(source.name[0] == '$') source.name[0] = '\0';
            else HashMap_insert(&(skd->sources_alias), source.name, iau);
            HashMap_insert(&(skd->sources), iau, &source);
        }
    }
    free(line);
    line = NULL;
    scan_idx = seek_to_section(stream, "$SKED");
    CLOSE_STREAM_ON_FAILURE(stream, scan_idx < 0, 1, "Schedule contains no $SKED section.");
    skd->scan_count = 0;
    while((line_len = getline(&line, &line_cap, stream)) != -1) {
        if(line[0] == '$') break;
        (skd->scan_count)++;
    }
    free(line);
    line = NULL;
    skd->scans = (Scan*) malloc(skd->scan_count * (sizeof(Scan) + skd->stations_ant.size + 1));
    failure = fseek(stream, 0, SEEK_SET);
    CLOSE_STREAM_ON_FAILURE(stream, failure, 1, "Unable to seek to beginning of schedule.");
    scan_idx = seek_to_section(stream, "$SKED");
    CLOSE_STREAM_ON_FAILURE(stream, scan_idx < 0, 1, "Failed to return to Schedule's $SKED section.");
    char timestamp_raw[12];
    char cable_wrap[skd->stations_ant.size * 2 + 1];
    Scan* current;
    size_t i = 0, j;
    while((line_len = getline(&line, &line_cap, stream)) != -1 && i < skd->scan_count) {
        if(line[0] == '$') break;
        current = Schedule_get_scan(*skd, i);
        ret = sscanf(line, " %8s %hu %*c%*c %*s %s %hu %*s %*u %*s %s %*s \n",
            current->source, &(current->cal_duration), timestamp_raw, &(current->obs_duration), cable_wrap);
        if(ret == 5) {
            failure = Datetime_parse_from_scan(&(current->timestamp), "y2d3h2m2s2", timestamp_raw);
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
    skd->scan_count = i;
    fclose(stream);
    return 0;
}