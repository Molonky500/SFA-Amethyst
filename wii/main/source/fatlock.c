#include "main.h"

typedef u32 mutex_t;

void _FAT_lock_init(mutex_t *mutex) {
    //printf("%s(%08X)\n", __FUNCTION__, (u32)mutex);
    OSMutex *osm = malloc(sizeof(OSMutex));
    if(!osm) {
        exiPrintf(" *** ERROR *** %s alloc fail\n", __FUNCTION__);
        return;
    }
    OSInitMutex(osm);
    *(OSMutex**)mutex = osm;
}

void _FAT_lock_deinit(mutex_t *mutex) {
    free(*(OSMutex**)mutex);
	return;
}

void _FAT_lock(mutex_t *mutex) {
	OSLockMutex(*(OSMutex**)mutex);
}

void _FAT_unlock(mutex_t *mutex) {
	OSUnlockMutex(*(OSMutex**)mutex);
}
