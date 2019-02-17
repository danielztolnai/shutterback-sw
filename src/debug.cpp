#include "debug.h"
#include <stdarg.h>
#include "RemoteDebug.h"

static RemoteDebug Debug;

void debug_setup()
{
    Debug.begin("shutter");
    Debug.setResetCmdEnabled(true);
    printd("Debugger initialized");
}

void debug_handle()
{
    Debug.handle();
}

void printd(const char *format, ...)
{
    char buf[512];
    va_list ap;

    va_start(ap, format);
    vsnprintf(buf, sizeof(buf), format, ap);
    va_end(ap);

    if (Debug.isActive(Debug.INFO)) {
        Debug.println(buf);
    }
}