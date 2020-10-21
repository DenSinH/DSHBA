#ifndef GC__SYSTEM_H
#define GC__SYSTEM_H

#include "Mem/Mem.h"
#include "ARM7TDMI/ARM7TDMI.h"

#include "default.h"
#include "flags.h"

#include "Scheduler/scheduler.h"
#include "Breakpoints/breakpoints.h"

class GBA {

#ifdef DO_DEBUGGER
public:
    bool paused{};

private:
    friend class Initializer;

    u32 stepcount = 0;
#   ifdef DO_BREAKPOINTS
    s_breakpoints breakpoints = {};
#   endif

#endif

public:
    s_scheduler scheduler = {};
    bool shutdown = false;

    Mem Memory;
    ARM7TDMI CPU = ARM7TDMI(&Memory);;

    GBA();
    ~GBA();
    void Run();

};

#endif //GC__SYSTEM_H
