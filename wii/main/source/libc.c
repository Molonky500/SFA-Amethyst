#include "main.h"

int __libogc_gettod_r(struct _reent *ptr, struct timeval *tp,
struct timezone *tz);
int __libogc_nanosleep(const struct timespec *tb,
struct timespec *rem);

OSMutex mallocMutex;

void initLibc() {
    OSInitMutex(&mallocMutex);
}

void *__syscall_sbrk_r(struct _reent *ptr, ptrdiff_t incr) {
	return _my_sbrk_r(ptr, incr);
}

int __syscall_lock_init(int *lock, int recursive) {
    //exiPrintf("%s(%08X, %d)\n", __FUNCTION__, (u32)lock, recursive);
	if(!lock) return -1;
	*lock = 0;

	OSMutex *retlck = malloc(sizeof(OSMutex));
    if(!retlck) return -1;

    //XXX recursive
	OSInitMutex(retlck);
    *lock = (int)retlck;
    return 0;
}

int __syscall_lock_close(int *lock) {
    //exiPrintf("%s(%08X)\n", __FUNCTION__, (u32)lock);
	if(!lock || *lock==0) return -1;
    free(*(OSMutex**)lock);
    *lock = 0;
    return 0;
}

int __syscall_lock_release(int *lock) {
	//exiPrintf("%s(%08X)\n", __FUNCTION__, (u32)lock);
	if(!lock || *lock==0) return -1;
    OSUnlockMutex(*(OSMutex**)lock);
    return 0;
}

int __syscall_lock_acquire(int *lock) {
	//exiPrintf("%s(%08X)\n", __FUNCTION__, (u32)lock);
	if(!lock || *lock==0) return -1;
    OSLockMutex(*(OSMutex**)lock);
    return 0;
}

void __syscall_malloc_lock(struct _reent *ptr) {
    //exiPrintf("%s(%08X)\n", __FUNCTION__, (u32)ptr);
    OSLockMutex(&mallocMutex);
}

void __syscall_malloc_unlock(struct _reent *ptr) {
	//exiPrintf("%s(%08X)\n", __FUNCTION__, (u32)ptr);
    OSUnlockMutex(&mallocMutex);
}

void __syscall_exit(int rc) {
	exiPrintf("%s(%08X)\n", __FUNCTION__, rc);
}

int  __syscall_gettod_r(struct _reent *ptr, struct timeval *tp, struct timezone *tz){
	return __libogc_gettod_r(ptr,tp,tz);
}

int __syscall_nanosleep(const struct timespec *req, struct timespec *rem){
	return __libogc_nanosleep(req, rem);
}
