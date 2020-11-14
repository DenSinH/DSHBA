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

public:
    bool Paused = false;
    bool Shutdown = false;
    void (*Interaction)() = nullptr;

    GBA();
    ~GBA();
    void Run();
    void Reset();

    void LoadROM(std::string file_path) {
        Memory.LoadROM(file_path);
        Reset();
    }

    void ReloadROM() {
        Memory.ReloadROM();
    }

    void LoadBIOS(std::string& file_path) {
        Memory.LoadBIOS(file_path);
    }

private:
    friend class Initializer;
    friend int main();

#ifdef DO_DEBUGGER
    u32 Stepcount = 0;

# ifdef DO_BREAKPOINTS
    s_breakpoints Breakpoints = {};
# endif

#endif

    s_scheduler Scheduler = s_scheduler(&CPU.timer);
    MMIO IO = MMIO(&PPU, &CPU, &Memory, &Scheduler);
    Mem Memory = Mem(&IO, &Scheduler, &CPU.pc, &CPU.timer, [&cpu = CPU]() -> void {
        cpu.PipelineReflush();
    });
    ARM7TDMI CPU = ARM7TDMI(&Scheduler, &Memory);
    GBAPPU PPU = GBAPPU(&Scheduler, &Memory);

};

#endif //GC__SYSTEM_H
