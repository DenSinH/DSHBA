#include "MMIO.h"

#include "Registers.h"
#include "IOFlags.h"

#include "const.h"

// needed for frame size data:
#include "../PPU/shaders/GX_constants.h"

MMIO::MMIO(GBAPPU* ppu, ARM7TDMI* cpu, s_scheduler* scheduler) {
    PPU = ppu;
    CPU = cpu;
    Scheduler = scheduler;

    HBlankFlag = {
        .callback = HBlankFlagEvent,
        .caller   = this,
        .time = 960 // (see below)
    };

    VBlankFlag = {
        .callback = VBlankFlagEvent,
        .caller   = this,
        .time     = CYCLES_PER_SCANLINE * VISIBLE_SCREEN_HEIGHT
    };

    add_event(scheduler, &HBlankFlag);
    add_event(scheduler, &VBlankFlag);

    ReadPrecall[static_cast<u32>(IORegister::DISPSTAT) >> 1]   = &MMIO::ReadDISPSTAT;
    WriteCallback[static_cast<u32>(IORegister::DISPSTAT) >> 1] = &MMIO::WriteDISPSTAT;

    ReadPrecall[static_cast<u32>(IORegister::KEYINPUT) >> 1]   = &MMIO::ReadKEYINPUT;
}

SCHEDULER_EVENT(MMIO::HBlankFlagEvent) {
    /*
     * HBlank IRQ is requrested after 960 cycles
     * HBlank flag is clear for 1006 cycles
     * Then for the rest of the scanline (226 cycles), HBlank flag is set
     * */
    auto IO = (MMIO*)caller;

    IO->DISPSTAT ^= static_cast<u16>(DISPSTATFlags::HBLank);
    if (IO->DISPSTAT & static_cast<u16>(DISPSTATFlags::HBLank)) {
        // HBlank was set, clear after 226 cycles
        event->time += 226;
    }
    else {
        // HBlank was cleared, clear after 1006 cycles
        event->time += 1006;
    }

    add_event(scheduler, event);
}

SCHEDULER_EVENT(MMIO::VBlankFlagEvent) {
    /*
     * VBlank is set after 160 scanlines, set for the rest of the frame
     * */
    auto IO = (MMIO*)caller;

    IO->DISPSTAT ^= static_cast<u16>(DISPSTATFlags::VBlank);
    if (IO->DISPSTAT & static_cast<u16>(DISPSTATFlags::VBlank)) {
        // VBlank was set, clear 68 scanlines (total frame height - visible frame height)
        event->time += CYCLES_PER_SCANLINE * (TOTAL_SCREEN_HEIGHT - VISIBLE_SCREEN_HEIGHT);
    }
    else {
        // VBlank was cleared, set after visible frame
        event->time += CYCLES_PER_SCANLINE * VISIBLE_SCREEN_HEIGHT;
    }

    add_event(scheduler, event);
}

READ_PRECALL(MMIO::ReadDISPSTAT) {
    return DISPSTAT;
}

WRITE_CALLBACK(MMIO::WriteDISPSTAT) {
    // VBlank/HBlank/VCount flags can not be written to
    DISPSTAT = (DISPSTAT &  ~0x7) | (value & 0x7);
}

u16 MMIO::ReadKEYINPUT() {
    // todo: poll KEYCNT for interrupts
    return KEYINPUT;
}