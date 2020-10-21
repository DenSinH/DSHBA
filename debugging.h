#ifndef GC__DEBUGGING_H
#define GC__DEBUGGING_H

#include <stdlib.h>

#include "default.h"

static u32 parsehex(const char* string) {
    u32 i = strtoul(string, NULL, 16);
    if (i == 0) {
        i = strtoul(string, NULL, 0);
    }
    return i;
}

static u32 parsedec(const char* string) {
    return (u32)strtoul(string, NULL, 10);
}

#endif //GC__DEBUGGING_H
