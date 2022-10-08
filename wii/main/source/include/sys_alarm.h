#ifndef __SYS_ALARM_H__
#define __SYS_ALARM_H__

#include "thread.h"
#define MAX_ALARMS 64

typedef u32 syswd_t;
typedef void (*alarmcallback)(syswd_t alarm,void *cb_arg);

typedef struct {
    bool used;
    OSAlarm alarm;
    alarmcallback realCb;
    void *cbParam;
} SysDolAlarm;

extern SysDolAlarm _alarms[MAX_ALARMS];

s32 SYS_CreateAlarm(syswd_t *thealarm);
s32 SYS_SetAlarm(syswd_t thealarm,
    const struct timespec *tp,
    alarmcallback cb,
    void *cbarg);
s32 SYS_SetPeriodicAlarm(syswd_t thealarm,
    const struct timespec *tp_start,
    const struct timespec *tp_period,
    alarmcallback cb,
    void *cbarg);
s32 SYS_RemoveAlarm(syswd_t thealarm);
s32 SYS_CancelAlarm(syswd_t thealarm);

#endif //__SYS_ALARM_H__
