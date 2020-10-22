#include <cstdio>

#include "system.h"
#include "sleeping.h"
#include "const.h"

#include "flags.h"

GBA::GBA() {
    scheduler   = { .timer = &CPU.timer };
    breakpoints = {};
    paused      = false;

    Memory.LoadBIOS(BIOS_FILE);

#ifdef DO_BREAKPOINTS
//    add_breakpoint(&breakpoints, 0x0803123c);
//    add_breakpoint(&breakpoints, 0x00000500);
//    add_breakpoint(&breakpoints, 0x8000586c);
#endif
}

GBA::~GBA() {

}

void GBA::Run() {
    CPU.SkipBIOS();

    while (!shutdown) {

        // todo: enable this once at least one event is in the scheduler at all times
//        if (should_do_events(&scheduler)) {
//            do_events(&scheduler);
//        }
        CPU.Step();

#if defined(DO_BREAKPOINTS) && defined(DO_DEBUGGER)
        if (check_breakpoints(&breakpoints, CPU.pc - ((CPU.CPSR & static_cast<u32>(CPSRFlags::T)) ? 4 : 8))) {
            log_debug("Hit breakpoint %08x", CPU.pc - ((CPU.CPSR & static_cast<u32>(CPSRFlags::T)) ? 4 : 8));
            paused = true;
        }
#endif

#ifdef DO_DEBUGGER
        while (paused && (stepcount == 0) && !shutdown) {
            // sleep for a bit to not have a busy wait. This is for debugging anyway, and we are frozen
            // so it's okay if it's not instant
            sleep_ms(16);
        }

        if (stepcount > 0) {
            stepcount--;
        }
#endif
    }
}