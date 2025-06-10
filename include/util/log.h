#ifndef __LOG_H__
#define __LOG_H__

#include <stdio.h>

#ifdef LOGGING
#define LOG_ERROR(msg) fprintf(stderr, "ERROR: %s\n", msg);
#define LOG_INFO(msg) printf("INFO: %s\n", msg);
#define CLOSE_STREAM_ON_FAILURE(stream, failure, val, msg) if(failure) {\
    fclose(stream);\
    LOG_ERROR(msg);\
    return val;\
}
#else
#define LOG_ERROR(msg)
#define LOG_INFO(msg)
#define CLOSE_STREAM_ON_FAILURE(stream, failure, val, msg) if(failure) {\
    fclose(stream);\
    return val;\
}
#endif

#endif /* __LOG_H__ */
