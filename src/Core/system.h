#ifndef GC__SYSTEM_H
#define GC__SYSTEM_H

#include "Memory/Memory.h"

#include "default.h"
#include "flags.h"

#include "Scheduler/scheduler.h"
#include "Breakpoints/breakpoints.h"

class GBA {

#ifdef DO_DEBUGGER
    public:
        bool paused{};

    private:
        u32 stepcount{};
#endif

    public:
        s_scheduler scheduler{};
        bool shutdown{};

        Memory memory;

        GBA();
        ~GBA();
        void Run();

#if defined(DO_BREAKPOINTS) || defined(DO_DEBUGGER)
    s_breakpoints breakpoints{};
#endif

};

#endif //GC__SYSTEM_H
