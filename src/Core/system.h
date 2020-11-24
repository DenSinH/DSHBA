#ifndef GC__SYSTEM_H
#define GC__SYSTEM_H

#include "Mem/Mem.h"
#include "ARM7TDMI/ARM7TDMI.h"
#include "PPU/PPU.h"
#include "APU/APU.h"
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

#ifdef DO_DEBUGGER
    u32 Stepcount = 0;

# ifdef DO_BREAKPOINTS
    s_breakpoints Breakpoints = {};
# endif

#endif

    i32 timer = 0;
    s_scheduler Scheduler = s_scheduler(&timer);

    MMIO IO = MMIO(
            &PPU,
            &APU,
            &CPU,
            &Memory,
            &Scheduler
    );

    Mem Memory = Mem(
            &IO,
            &Scheduler,
            CPU.Registers,
            &CPU.CPSR,
            &timer,
            [&cpu = CPU]() -> void {
                    cpu.PipelineReflush();
            }
    );

    GBAAPU APU = GBAAPU(
            &Scheduler,
            IO.GetWaveRAM_ptr(),
            std::function<void(u32)> ([this](u32 addr){ IO.TriggerAudioDMA(addr); })
    );

    ARM7TDMI CPU = ARM7TDMI(
            &Scheduler,
            &Memory
    );

    GBAPPU PPU = GBAPPU(
            &Scheduler,
            &Memory
    );
};

#endif //GC__SYSTEM_H
