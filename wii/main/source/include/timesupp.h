#ifndef __TIMESUPP_H__
#define __TIMESUPP_H__

#define TB_REQ						250
#define TB_SUCCESSFUL				0

#define TB_SECSPERMIN				60
#define TB_MINSPERHR				60
#define TB_MONSPERYR				12
#define TB_DAYSPERYR				365
#define TB_HRSPERDAY				24
#define TB_SECSPERDAY				(TB_SECSPERMIN*TB_MINSPERHR*TB_HRSPERDAY)
#define TB_SECSPERNYR				(365*TB_SECSPERDAY)
#define TB_MSPERSEC					1000
#define TB_USPERSEC					1000000
#define TB_NSPERSEC					1000000000
#define TB_NSPERMS					1000000
#define TB_NSPERUS					1000
#define TB_USPERTICK				10000

#define diff_ticks(tick0,tick1)		(((u64)(tick1)<(u64)(tick0))?((u64)-1-(u64)(tick0)+(u64)(tick1)):((u64)(tick1)-(u64)(tick0)))

u32 gettick(void);
u64 gettime(void);
void settime(u64 t);
u32 diff_sec(u64 start,u64 end);
u32 diff_msec(u64 start,u64 end);
u32 diff_usec(u64 start,u64 end);
u32 diff_nsec(u64 start,u64 end);
void __timesystem_init(void);
void timespec_subtract(const struct timespec *tp_start,
    const struct timespec *tp_end,struct timespec *result);
unsigned long long timespec_to_ticks(const struct timespec *tp);
void udelay(unsigned us);
int __libogc_nanosleep(const struct timespec *tb,
    struct timespec *rem);
clock_t clock(void);
int __libogc_gettod_r(struct _reent *ptr, struct timeval *tp,
    struct timezone *tz);

#endif
