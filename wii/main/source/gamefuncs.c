#include "main.h"

//pointers to functions in the game binary
u32       (*aramAllocAndUpload)(u16 *buf,int size) = 0x80284468;
bool      (*audioInit)(void) = 0x80009bf8;
bool      (*audioLoadSdiFile)(void *sdiFile, void *samFile) = 0x802744bc;
void      (*audioPlaySound)(void *sourceObj,int soundid) = 0x8000bb18;
BOOL      (*audioStopSound)(uint id) = 0x80272868;
void      (*debugPrintf)(const char *fmt, ...) = 0x801378a8;
void*     (*gameAlloc)(u32 size, u32 tag, const char *name) = 0x80023cc8;
void      (*gameFree)(void *addr) = 0x800233e8;
void*     (*getArenaStart)(void) = 0x802416f8;
void*     (*heapInit)(void *addr,uint size,int slots) = 0x80023ed4;
void      (*InsertAlarm)(OSAlarm *alarm, OSTime time, OSAlarmHandler handler) = 0x80240d8c;
void*     (*OSAllocFromHeap)(OSHeapHandle heap,u32 size) = 0x8024148c;
void      (*OSCancelAlarm)(OSAlarm *alarm) = 0x80241044;
void      (*OSCancelThread)(OSThread *thread) = 0x802464ac;
void      (*OSCreateAlarm)(OSAlarm *alarm) = 0x80240d80;
BOOL      (*OSCreateThread)(OSThread *thread, void *(*func)(void*), void *param, void *stackBase, u32 stackSize, OSPriority priority, u16 attribute) = 0x802462a8;
int       (*OSDisableInterrupts)(void) = 0x8024377c;
int       (*OSDisableScheduler)(void) = 0x80245d94;
void      (*__OSDispatchInterrupt)(int, OSContext*) = 0x80243c54;
int       (*OSEnableInterrupts)(void) = 0x80243790;
int       (*OSEnableScheduler)(void) = 0x80245dd4;
void*     (*OSGetArenaHi)(void) = 0x802416f0;
OSThread* (*OSGetCurrentThread)(void) = 0x80245d88;
int       (*__OSGetEffectivePriority)(OSThread*) = 0x80245e7c;
u64       (*OSGetTime)(void) = 0x80246c50;
void      (*OSInitMessageQueue)(OSMessageQueue* mq, OSMessage* msgArray, s32 msgCount) = 0x80244000;
void      (*OSInitThreadQueue)(OSThreadQueue* queue) = 0x80245d78;
void      (*__OSInterruptInit)(void) = 0x802437f8;
//BOOL      (*OSJamMessage)(OSMessageQueue *mq, OSMessage msg, s32 flags); //not present
void      (*OSLoadContext)(OSContext*) = 0x80242394;
BOOL      (*OSReceiveMessage)(OSMessageQueue *mq, OSMessage *msg, s32 flags) = 0x80244128;
void      (*__OSReschedule)(void) = 0x80246278;
int       (*OSRestoreInterrupts)(int) = 0x802437a4;
s32       (*OSResumeThread)(OSThread* thread) = 0x80246668;
BOOL      (*OSSendMessage)( OSMessageQueue *mq, OSMessage msg, s32 flags) = 0x80244060;
void      (*OSSetAlarm)(OSAlarm *alarm, OSTime tick, OSAlarmHandler handler) = 0x80240fdc;
u32       (*__OSSetExceptionHandler)(u32 exception, void *handler) = 0x80240bc4;
BOOL      (*OSSetThreadPriority)(OSThread* thread, OSPriority priority) = 0x80245eb8;
void      (*OSSleepThread)(OSThreadQueue* queue) = 0x80246a60;
s32       (*OSSuspendThread)(OSThread* thread) = 0x802468f0;
void      (*OSWakeupThread)(OSThreadQueue* queue) = 0x80246b4c;
void      (*SelectThread)(BOOL) = 0x80246078;
OSThread* (*SetEffectivePriority)(OSThread*, int) = 0x80245eb8; //XXX same as OSSetThreadPriority
