#include "main.h"

void (*OSCreateAlarm)(OSAlarm *alarm) = 0x80240d80;
void (*OSSetAlarm)(OSAlarm*       alarm,
                OSTime         tick,
                OSAlarmHandler handler) = 0x80240fdc;
