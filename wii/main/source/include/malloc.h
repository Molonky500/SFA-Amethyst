#ifndef __MALLOC_H__
#define __MALLOC_H__

#include "gcc-macros.h"
#include "string.h"
extern size_t __malloc_margin;
extern char *__brkval;

#define HEAP_CANARY_SIZE 64

#define _arena2_min 0x92000000
#define _arena2_max 0x93000000
#define _mem2_heap_start (*(char**)0x80003124)
#define _mem2_heap_end   (*(char**)0x80003128)

//gcc expects malloc() to return an address aligned to 8 bytes
//(16 on a 64-bit system) and makes optimizations based on that
//assumption, which will cause bugs if it's not met.
//This must be a power of two!
#define MALLOC_ALIGN 16

WEAK COLD void* on_malloc_fail(size_t len);
MALLOC MUST_CHECK void* malloc(size_t len);
MALLOC MUST_CHECK void* calloc(size_t num, size_t size);
                  void  free(void *p);
       MUST_CHECK void* realloc(void *ptr, size_t len);

#endif //__MALLOC_H__
