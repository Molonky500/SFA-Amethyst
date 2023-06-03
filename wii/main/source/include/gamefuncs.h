#ifndef __GAMEFUNCS_H__
#define __GAMEFUNCS_H__

extern u32       (*aramAllocAndUpload)(u16 *buf,int size);
extern bool      (*audioInit)(void);
extern bool      (*audioLoadSdiFile)(void *sdiFile, void *samFile);
extern void      (*audioPlaySound)(void *sourceObj,int soundid);
extern BOOL      (*audioStopSound)(uint id);
extern void      (*debugPrintf)(const char *fmt, ...);
extern void*     (*gameAlloc)(u32 size, u32 tag, const char *name);
extern void      (*gameFree)(void *addr);
extern void      (*InsertAlarm)(OSAlarm *alarm, OSTime time, OSAlarmHandler handler);
extern void      (*OSCancelAlarm)(OSAlarm *alarm);
extern void      (*OSCancelThread)(OSThread *thread);
extern void      (*OSCreateAlarm)(OSAlarm *alarm);
extern BOOL      (*OSCreateThread)(OSThread *thread, void *(*func)(void*), void *param, void *stackBase, u32 stackSize, OSPriority priority, u16 attribute);
extern int       (*OSDisableInterrupts)(void);
extern int       (*OSDisableScheduler)(void);
extern void      (*__OSDispatchInterrupt)(int, OSContext*);
extern int       (*OSEnableInterrupts)(void);
extern int       (*OSEnableScheduler)(void);
extern OSThread* (*OSGetCurrentThread)(void);
extern u64       (*OSGetTime)(void);
extern void      (*OSInitMessageQueue)(OSMessageQueue* mq, OSMessage* msgArray, s32 msgCount);
extern void      (*OSInitThreadQueue)(OSThreadQueue* queue);
extern void      (*__OSInterruptInit)(void);
//extern BOOL      (*OSJamMessage)(OSMessageQueue *mq, OSMessage msg, s32 flags); //not present
extern void      (*OSLoadContext)(OSContext*);
extern BOOL      (*OSReceiveMessage)(OSMessageQueue *mq, OSMessage *msg, s32 flags);
extern void      (*__OSReschedule)(void);
extern int       (*OSRestoreInterrupts)(int);
extern s32       (*OSResumeThread)(OSThread* thread);
extern BOOL      (*OSSendMessage)( OSMessageQueue *mq, OSMessage msg, s32 flags);
extern void      (*OSSetAlarm)(OSAlarm *alarm, OSTime tick, OSAlarmHandler handler);
extern u32       (*__OSSetExceptionHandler)(u32 exception, void *handler);
extern BOOL      (*OSSetThreadPriority)(OSThread* thread, OSPriority priority);
extern void      (*OSSleepThread)(OSThreadQueue* queue);
extern s32       (*OSSuspendThread)(OSThread* thread);
extern void      (*OSWakeupThread)(OSThreadQueue* queue);
extern void      (*SelectThread)(BOOL);

#endif //__GAMEFUNCS_H__
