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
    uint tmp, val[strlen(ord)];
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
    return 0;
}

float Datetime_to_mjd(Datetime dt) {
    float yyyy, ddd, hh, mm, ss;
    yyyy = (float) dt.yrs; ddd = (float) dt.day;
    hh = (float) dt.hrs; mm = (float) dt.min; ss = (float) dt.sec;
    float jd = 367.f * yyyy - floorf(7.f * (yyyy + floorf((mm + 9.f) / 12.f)) / 4.f) + \
        (275.f * mm) / 9.f + ddd + 1721013.5 + hh / 24.f + mm / 1440.f + ss / 86400.f;
    return jd - 2400000.5;
}

#endif /* MJD_H */