#ifndef MJD_H
#define MJD_H

#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "sofa/sofa.h"
#include "sofa/sofam.h"
#include "log.h"

typedef struct {
    uint16_t yrs, day;
    uint8_t hrs, min;
    uint16_t sec;
} Datetime;

// Accepts a format string of the form "y2d3h2m2s2"
// Every character pair represents a field and the number of digits in the string it occupies.
// This function is used for compressed dates like 2025311120000 ("y4d3h2m2s2")
// Fields can be omitted but not repeated
#pragma GCC diagnostic ignored "-Wunused-function"
static unsigned int Datetime_parse_from_scan(Datetime* dt, const char* format, const char* line) { // TODO: This needs refinement
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
    // TODO: It might be possible to just stuff all variadic arguments into sscanf regardless of how many are used
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
            case 's': dt->sec = (uint16_t) tmp; break;
            default:
                LOG_ERROR("Invalid Datetime component symbol.");
                return 1;
        }
    }
    // printf("%s: %hu %hu %hhu %hhu %hhu\n", line, dt->yrs, dt->day, dt->hrs, dt->min, dt->sec);
    return 0;
}

#pragma GCC diagnostic ignored "-Wunused-function"
static Datetime Datetime_add_seconds(Datetime dt, uint16_t secs) {
    dt.sec += secs;
    uint16_t days_per_year;
    while(dt.sec > 60) {
        dt.sec -= 60;
        dt.min++;
        while(dt.min > 60) {
            dt.min -= 60;
            dt.hrs++;
            while(dt.hrs > 24) {
                dt.hrs -= 24;
                dt.day++;
                while(1) {
                    days_per_year = ((dt.yrs % 4 == 0 && dt.yrs % 100 != 0) || (dt.yrs % 400 == 0)) ? 366 : 365;
                    if(dt.day > days_per_year) {
                        dt.day -= days_per_year;
                        dt.yrs++;
                    } else {
                        break;
                    }
                }
            }
        }
    }    
    return dt;
}

#pragma GCC diagnostic ignored "-Wunused-function"
static double Datetime_to_jd(Datetime dt) {
    double jd1, jd2, jd;
    iauCal2jd((int) dt.yrs, 1, 1, &jd1, &jd2);
    jd = jd1 + jd2 + ((double) dt.day) - 1.0;
    jd += (((double) dt.hrs) / 24.0);
    jd += (((double) dt.min) / 1440.0);
    jd += (((double) dt.sec) / 86400.0);
    return jd;
}

#pragma GCC diagnostic ignored "-Wunused-function"
static inline double Datetime_to_mjd(Datetime dt) {
    return Datetime_to_jd(dt) - 2400000.5;
}

#pragma GCC diagnostic ignored "-Wunused-function"
static inline double jd2gmst(double jd) {
    // Adapted from https://www.mathworks.com/matlabcentral/fileexchange/28176-julian-date-to-greenwich-mean-sidereal-time
    // by Darin Koblick
    double jd0, hrs, min, max;
    min = floor(jd) - 0.5;
    max = floor(jd) + 0.5;
    jd0 = (jd > max) ? max : min;
    hrs = \
        (6.697374558) + \
        (0.06570982441908 * (jd0 - 2451545.0)) + \
        (1.00273790935 * (jd - jd0) * 24.0) + \
        (0.000026 * pow((jd - 2451545.0) / 36525.0, 2.0));
    return fmod(hrs, 24.0) * 15.0;
}

#pragma GCC diagnostic ignored "-Wunused-function"
static inline double Datetime_to_gmst(Datetime dt) {
    return jd2gmst(Datetime_to_jd(dt));
}

#endif /* MJD_H */
