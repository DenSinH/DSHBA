#include <cstdio>

#include "system.h"
#include "sleeping.h"
#include "const.h"

#include "flags.h"

GBA::GBA() {
    Scheduler.timer = &CPU.timer;

    Memory.LoadBIOS(BIOS_FILE);

#ifdef DO_BREAKPOINTS
    Breakpoints = {};
    Paused      = false;

    // add_breakpoint(&Breakpoints, 0x08020af4);
    // add_breakpoint(&Breakpoints, 0x00000f60);
    // add_breakpoint(&Breakpoints, 0x000000128);
//    add_breakpoint(&Breakpoints, 0x08000134);
#endif
}

GBA::~GBA() {

}

void GBA::Run() {
    CPU.SkipBIOS();

    while (!Shutdown) {

        if (unlikely(should_do_events(&Scheduler))) {
            do_events(&Scheduler);
        }
        CPU.Step();

#if defined(DO_BREAKPOINTS) && defined(DO_DEBUGGER)
        if (check_breakpoints(&Breakpoints, CPU.pc - ((CPU.CPSR & static_cast<u32>(CPSRFlags::T)) ? 4 : 8))) {
            log_debug("Hit breakpoint %08x", CPU.pc - ((CPU.CPSR & static_cast<u32>(CPSRFlags::T)) ? 4 : 8));
            Paused = true;
        }
#endif

#ifdef DO_DEBUGGER
        while (Paused && (Stepcount == 0) && !Shutdown) {
            // sleep for a bit to not have a busy wait. This is for debugging anyway, and we are frozen
            // so it's okay if it's not instant
            sleep_ms(16);
        }

        if (Stepcount > 0) {
            Stepcount--;
        }
#endif
    }
}