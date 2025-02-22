#ifndef SKD_H
#define SKD_H

#include <stddef.h>
#include "util/shaders.h"
#include "util/mjd.h"
#include "util/hashmap.h"

// represents both station's and sources
// the x-component is an untagged union
typedef struct {
    char name[9];
    union { float lam; float alf; };
    float phi;
} NamedPoint;
// Representation of a single scan entry
// NOTE: It isn't safe to dereference this due to the flexible array member
typedef struct {
    Datetime timestamp;
    uint16_t cal_duration, obs_duration;
    char source[9];
    uint16_t* scan_offsets;
    char ids[];
} ScanFAM;
// Schedule data gets further parsed in the SchedulePass
typedef struct {
    HashMap stations_ant;
    HashMap stations_pos;
    // HashMap sources_sat;
    HashMap sources;
    HashMap sources_alias;
    size_t scan_count;
    ScanFAM* scans;
} Schedule;
// checks for inconsistencies across Schedule's various HashMaps
unsigned int Schedule_debug_and_validate(Schedule skd, unsigned int display);
// required because of the flexible array member
ScanFAM* Schedule_get_scan(Schedule skd, size_t i);
// free Schedule
void Schedule_free(Schedule skd);
// initialize from a skd file
unsigned int Schedule_build_from_source(Schedule* skd, const char* path);

#endif /* SKD_H */
