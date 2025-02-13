#ifndef MJD_H
#define MJD_H

#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "log.h"

typedef struct {
    uint16_t yrs, day;
    uint8_t hrs, min, sec, __p0;
} Datetime;

// Accepts a format string of the form "y2d3h2m2s2"
// Every character pair represents a field and the number of digits in the string it occupies.
// This function is used for compressed dates like 2025311120000 ("y4d3h2m2s2")
// Fields can be omitted but not repeated
unsigned int Datetime_parse_from_obs(Datetime* dt, const char* format, const char* line) {
    char ord[6] = {'\0',}, fmt[16] = {'\0',};
    size_t i, j;
    for(i = 0; i < strlen(format) / 2; ++i) {
        j = strlen(ord);
        ord[j] = format[i * 2];
        ord[j + 1] = '\0';
        j = strlen(fmt);
        strcpy(&(fmt[j]), "%*u");
        fmt[j + 1] = format[i * 2 + 1];
    }
    unsigned int tmp, val[strlen(ord)];
    int ret;
    switch(strlen(ord)) {
        case 0:
            LOG_ERROR("Unable to parse Datetime format string.");
            return 1;
        case 1: ret = sscanf(line, fmt, &(val[0])); break;
        case 2: ret = sscanf(line, fmt, &(val[0]), &(val[1])); break;
        case 3: ret = sscanf(line, fmt, &(val[0]), &(val[1]), &(val[2])); break;
        case 4: ret = sscanf(line, fmt, &(val[0]), &(val[1]), &(val[2]), &(val[3])); break;
        case 5: ret = sscanf(line, fmt, &(val[0]), &(val[1]), &(val[2]), &(val[3]), &(val[4])); break;
        default:
            LOG_ERROR("Too many Datetime format specifiers provided.");
            return 1;
    }
    j = 0;
    if(ret != (int) strlen(ord)) {
        LOG_ERROR("Failed to parse Datetime.");
        return 1;
    } else for(i = 0; i < strlen(ord); ++i) {
        tmp = val[j++];
        switch(ord[i]) {
            case 'y': dt->yrs = (uint16_t) tmp; break;
            case 'd': dt->day = (uint16_t) tmp; break;
            case 'h': dt->hrs = (uint8_t) tmp; break;
            case 'm': dt->min = (uint8_t) tmp; break;
            case 's': dt->sec = (uint8_t) tmp; break;
            default:
                LOG_ERROR("Invalid Datetime component symbol.");
                return 1;
        }
    }
    // printf("%s: %hu %hu %hhu %hhu %hhu\n", line, dt->yrs, dt->day, dt->hrs, dt->min, dt->sec);
    return 0;
}

float Datetime_to_mjd(Datetime dt) {
    float yrs, day, hrs, min, sec;
    yrs = (float) dt.yrs; day = (float) dt.day;
    hrs = (float) dt.hrs; min = (float) dt.min; sec = (float) dt.sec;
    float jd = 1721013.5 + 367.f * yrs - floorf(7.f * (yrs + floorf((day + 9.f) / 12.f)) / 4.f) + \
        day + hrs / 24.f + min / 1440.f + sec / 86400.f;
    return jd - 2400000.5;
}

float Datetime_greenwich_sidereal_time(Datetime dt) {
    float jc1 = (Datetime_to_mjd(dt) - 51544.5f) / 36525.f;
    float jc2 = jc1 * jc1;
    float jc3 = jc2 * jc1;
    float gst = 100.46061837 + 36000.770053608 * jc1 + 0.000387933 * jc2 - jc3 / 38710000.f;
    return fmod(gst, 360.f);
}

#endif /* MJD_H */