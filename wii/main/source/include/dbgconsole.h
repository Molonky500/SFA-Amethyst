#ifndef _DBGCONSOLE_H_
#define _DBGCONSOLE_H_

typedef struct _DebugConsoleCommand {
    const char *cmd;
    const char *help;
    void (*func)(char*);
} DebugConsoleCommand;

#endif //_DBGCONSOLE_H_
