#ifndef MJD_H
#define MJD_H

#include <math.h>

#include "log.h"

typedef struct {
    uint16_t yyyy, ddd;
    uint8_t hh, mm;
    float ss;
} Datetime;

unsigned int Datetime_parse_from_obs(Datetime* dt, const char* fmt, const char* line) {
    int ret = sscanf(line, fmt, &(dt->yyyy), &(dt->ddd), &(dt->hh), &(dt->mm), &(dt->ss));
    if(ret != 5) {
        LOG_ERROR("Failed to parse Datetime.");
        return 1;
    }
    return 0;
}

float Datetime_to_mjd(Datetime dt) {
    float yyyy, ddd, hh, mm;
    yyyy = (float) dt.yyyy; ddd = (float) dt.ddd;
    hh = (float) dt.hh; mm = (float) dt.mm;
    float jd = 367.f * yyyy - floorf(7.f * (yyyy + floorf((mm + 9.f) / 12.f)) / 4.f) + \
        (275.f * mm) / 9.f + ddd + 1721013.5 + hh / 24.f + mm / 1440.f + dt.ss / 86400.f;
    return jd - 2400000.5;
}

#endif /* MJD_H */