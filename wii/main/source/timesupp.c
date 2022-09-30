#include <_ansi.h>
#include <_syslist.h>

#include "main.h"
/*#include "processor.h"
#include "lwp.h"
#include "lwp_threadq.h"
#include "timesupp.h"
#include "exi.h"
#include "system.h"
#include "conf.h"*/

#include <stdio.h>
#include <sys/time.h>

/* time variables */
//static u32 exi_wait_inited = 0;
//static lwpq_t time_exi_wait;

//extern u32 __SYS_GetRTC(u32 *gctime);
//extern syssram* __SYS_LockSram(void);
//extern u32 __SYS_UnlockSram(u32 write);


u32 gettick(void) {
	u32 result;
	__asm__ __volatile__ (
		"mftb	%0\n"
		: "=r" (result)
	);
	return result;
}

u64 gettime(void) {
	u32 tmp;
	union uulc {
		u64 ull;
		u32 ul[2];
	} v;

	__asm__ __volatile__(
		"1:	mftbu	%0\n\
		    mftb	%1\n\
		    mftbu	%2\n\
			cmpw	%0,%2\n\
			bne		1b\n"
		: "=r" (v.ul[0]), "=r" (v.ul[1]), "=&r" (tmp)
	);
	return v.ull;
}

void settime(u64 t) {
	u32 tmp;
	union uulc {
		u64 ull;
		u32 ul[2];
	} v;

	v.ull = t;
	__asm__ __volatile__ (
		"li		%0,0\n\
		 mttbl  %0\n\
		 mttbu  %1\n\
		 mttbl  %2\n"
		 : "=&r" (tmp)
		 : "r" (v.ul[0]), "r" (v.ul[1])
	);
}

u32 diff_sec(u64 start,u64 end) {
	u64 diff;

	diff = diff_ticks(start,end);
	return OSTicksToSeconds(diff);
}

u32 diff_msec(u64 start,u64 end) {
	u64 diff;

	diff = diff_ticks(start,end);
	return OSTicksToMilliseconds(diff);
}

u32 diff_usec(u64 start,u64 end) {
	u64 diff;

	diff = diff_ticks(start,end);
	return OSTicksToMicroseconds(diff);
}

u32 diff_nsec(u64 start,u64 end) {
	u64 diff;

	diff = diff_ticks(start,end);
	return OSTicksToNanoseconds(diff);
}

void __timesystem_init(void) {
	/*if(!exi_wait_inited) {
		exi_wait_inited = 1;
		LWP_InitQueue(&time_exi_wait);
	}*/
}

void timespec_subtract(const struct timespec *tp_start,
const struct timespec *tp_end,struct timespec *result) {
	struct timespec start_st = *tp_start;
	struct timespec *start = &start_st;
	u32 nsecpersec = TB_NSPERSEC;

	if(tp_end->tv_nsec<start->tv_nsec) {
		int secs = (start->tv_nsec - tp_end->tv_nsec)/nsecpersec+1;
		start->tv_nsec -= nsecpersec * secs;
		start->tv_sec += secs;
	}
	if((tp_end->tv_nsec - start->tv_nsec)>nsecpersec) {
		int secs = (start->tv_nsec - tp_end->tv_nsec)/nsecpersec;
		start->tv_nsec += nsecpersec * secs;
		start->tv_sec -= secs;
	}

	result->tv_sec = (tp_end->tv_sec - start->tv_sec);
	result->tv_nsec = (tp_end->tv_nsec - start->tv_nsec);
}

unsigned long long timespec_to_ticks(const struct timespec *tp) {
	u64 ticks;
	ticks = OSSecondsToTicks(tp->tv_sec);
	ticks += OSNanosecondsToTicks(tp->tv_nsec);
	return ticks;
}

// this function spins till timeout is reached
void udelay(unsigned us) {
	unsigned long long start, end;
	start = gettime();
	while (1)
	{
		end = gettime();
		if (diff_usec(start,end) >= us)
			break;
	}
}

static volatile int _waitAlarm = 0;

static void _alarmHandler(OSAlarm *alarm, OSContext *ctx) {
	_waitAlarm = 0;
}

int __libogc_nanosleep(const struct timespec *tb,
struct timespec *rem) {

	_osDisableThreadSwitching = 1;
	_waitAlarm = 1;

	u64 timeout = timespec_to_ticks(tb);
	OSAlarm alarm;
	OSCreateAlarm(&alarm);
	OSSetAlarm(&alarm, timeout, _alarmHandler);
	while(_waitAlarm);

	_osDisableThreadSwitching = 0;
	return TB_SUCCESSFUL;
}

clock_t clock(void) {
	return -1;
}

int __libogc_gettod_r(struct _reent *ptr, struct timeval *tp,
struct timezone *tz) {
	u64 now;
	if (tp != NULL) {
		now = gettime();
		tp->tv_sec = OSTicksToSeconds(now) + 946684800;
		tp->tv_usec = OSTicksToMicroseconds(now) % TB_USPERSEC;
	}
	if (tz != NULL) {
		tz->tz_minuteswest = 0;
		tz->tz_dsttime = 0;

	}
	return 0;
}

