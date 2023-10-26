#include "main.h"
//compatibility layer for LWP -> Dolphin alarm handling

SysDolAlarm _alarms[MAX_ALARMS];

static void _compatAlarmCb(OSAlarm *alarm, OSContext *ctx) {
    for(int i=0; i<MAX_ALARMS; i++) {
        SysDolAlarm *al = &_alarms[i];
        if(alarm == &al->alarm) {
            al->realCb(i, al->cbParam);
            return;
        }
    }
    exiPrintf("uhoh, %s has alarm=%p\n", __FUNCTION__, alarm);
    PANIC("Can't find alarm!");
}

s32 SYS_CreateAlarm(syswd_t *thealarm)
{
	SysDolAlarm *alarm;
    int idx;
    for(int i=0; i<MAX_ALARMS; i++) {
        if(!_alarms[i].used) {
            alarm = &_alarms[i];
            idx = i;
            break;
        }
    }
    if(!alarm) {
        exiPrintf(" *** %s: no more alarms\n", __FUNCTION__);
        return -ENOMEM;
    }
    alarm->used = true;
	alarm->realCb = NULL;
	alarm->cbParam = NULL;
    OSCreateAlarm(&alarm->alarm);

	*thealarm = idx;
	//__lwp_thread_dispatchenable();
    //OSEnableScheduler();
	return 0;
}

s32 SYS_SetAlarm(syswd_t thealarm,
const struct timespec *tp,
alarmcallback cb,
void *cbarg)
{
	SysDolAlarm *alarm = &_alarms[thealarm];
	if(!alarm->used) return -EINVAL;

	alarm->cbParam = cbarg;
	alarm->realCb  = cb;
    OSSetAlarm(&alarm->alarm, timespec_to_ticks(tp), _compatAlarmCb);
	//__lwp_thread_dispatchenable();
    //OSEnableScheduler();
	return 0;
}

s32 SYS_SetPeriodicAlarm(syswd_t thealarm,
const struct timespec *tp_start,
const struct timespec *tp_period,
alarmcallback cb,
void *cbarg)
{
	SysDolAlarm *alarm = &_alarms[thealarm];
	if(!alarm->used) return -EINVAL;

	alarm->cbParam = cbarg;
	alarm->realCb  = cb;
    OSSetPeriodicAlarm(&alarm->alarm,
        timespec_to_ticks(tp_start),
        timespec_to_ticks(tp_period),
        _compatAlarmCb);
	//OSEnableScheduler();
	return 0;
}

s32 SYS_RemoveAlarm(syswd_t thealarm)
{
	SysDolAlarm *alarm = &_alarms[thealarm];
	if(!alarm->used) return -EINVAL;
    alarm->used = false;
    OSCancelAlarm(&alarm->alarm);
	//OSEnableScheduler();
	return 0;
}

s32 SYS_CancelAlarm(syswd_t thealarm)
{
	SysDolAlarm *alarm = &_alarms[thealarm];
	if(!alarm->used) return -EINVAL;
    OSCancelAlarm(&alarm->alarm);
	//OSEnableScheduler();
	return 0;
}
