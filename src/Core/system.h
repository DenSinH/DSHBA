#ifndef GC__SYSTEM_H
#define GC__SYSTEM_H

#include "Mem/Mem.h"
#include "ARM7TDMI/ARM7TDMI.h"
#include "PPU/PPU.h"
#include "IO/MMIO.h"

#include "default.h"
#include "flags.h"

#include "Scheduler/scheduler.h"
#include "Breakpoints/breakpoints.h"

class GBA {

#ifdef DO_DEBUGGER
public:
    bool Paused = false;

private:
    friend class Initializer;

    u32 Stepcount = 0;
# ifdef DO_BREAKPOINTS
    s_breakpoints Breakpoints = {};
# endif

#endif

public:
    s_scheduler Scheduler = {};
    bool Shutdown = false;

    MMIO IO = MMIO(&PPU, &CPU, &Memory, &Scheduler);
    Mem Memory = Mem(&IO, &CPU.pc, [&cpu = CPU]() -> void {
        cpu.PipelineReflush();
    });
    ARM7TDMI CPU = ARM7TDMI(&Scheduler, &Memory);
    GBAPPU PPU = GBAPPU(&Scheduler, &Memory);

    GBA();
    ~GBA();
    void Run();
};

#endif //GC__SYSTEM_H
