#ifndef __ACTION_H__
#define __ACTION_H__

// series of actions that can be pass to SchedulePass_handle_controls
typedef enum {
    ACTION_NONE = 0,
    ACTION_SKD_PASS_FASTER,
    ACTION_SKD_PASS_SLOWER,
    ACTION_SKD_PASS_PAUSE,
    ACTION_SKD_PASS_RESET
} Action;

#endif /* __ACTION_H__ */
