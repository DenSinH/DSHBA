#include "MMIO.h"

#include "Registers.h"
#include "IOFlags.h"
#include "Interrupts.h"

#include "const.h"

#include "../ARM7TDMI/ARM7TDMI.h"

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
    ReadPrecall[static_cast<u32>(IORegister::VCOUNT) >> 1]   = &MMIO::ReadVCount;

    ReadPrecall[static_cast<u32>(IORegister::KEYINPUT) >> 1]   = &MMIO::ReadKEYINPUT;

    WriteCallback[static_cast<u32>(IORegister::IME) >> 1] = &MMIO::WriteIME;
    WriteCallback[static_cast<u32>(IORegister::IE) >> 1] = &MMIO::WriteIE;
    WriteCallback[static_cast<u32>(IORegister::IF) >> 1] = &MMIO::WriteIF;
    ReadPrecall[static_cast<u32>(IORegister::IF) >> 1] = &MMIO::ReadIF;
}

SCHEDULER_EVENT(MMIO::HBlankFlagEvent) {
    /*
     * HBlank IRQ is requrested after 960 cycles <- todo
     * HBlank flag is clear for 1006 cycles
     * Then for the rest of the scanline (226 cycles), HBlank flag is set
     * */
    auto IO = (MMIO*)caller;

    IO->DISPSTAT ^= static_cast<u16>(DISPSTATFlags::HBLank);
    if (IO->DISPSTAT & static_cast<u16>(DISPSTATFlags::HBLank)) {
        // HBlank was set, clear after 226 cycles
        event->time += 226;

        // HBlank interrupts
        if (IO->DISPSTAT & static_cast<u16>(DISPSTATFlags::HBLankIRQ)) {
            IO->CPU->IF |= static_cast<u16>(Interrupt::HBlank);
            IO->CPU->ScheduleInterruptPoll();
        }
    }
    else {
        // HBlank was cleared, clear after 1006 cycles
        event->time += 1006;

        // increment VCount
        IO->VCount++;

        if (IO->VCount == TOTAL_SCREEN_HEIGHT) {
            IO->VCount = 0;
        }

        IO->CheckVCountMatch();
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
        // VBlank was set, clear after 68 scanlines (total frame height - visible frame height)
        event->time += CYCLES_PER_SCANLINE * (TOTAL_SCREEN_HEIGHT - VISIBLE_SCREEN_HEIGHT);

        // VBlank interrupts
        if (IO->DISPSTAT & static_cast<u16>(DISPSTATFlags::VBlankIRQ)) {
            IO->CPU->IF |= static_cast<u16>(Interrupt::VBlank);
            IO->CPU->ScheduleInterruptPoll();
        }
    }
    else {
        // VBlank was cleared, set after visible frame
        event->time += CYCLES_PER_SCANLINE * VISIBLE_SCREEN_HEIGHT;
    }

    add_event(scheduler, event);
}

void MMIO::CheckVCountMatch() {
    // VCount interrupts
    if (DISPSTAT & static_cast<u16>(DISPSTATFlags::VCountIRQ)) {
        if ((DISPSTAT >> 8) == VCount) {
            CPU->IF |= static_cast<u16>(Interrupt::VCount);
            CPU->ScheduleInterruptPoll();
        }
    }
}

READ_PRECALL(MMIO::ReadDISPSTAT) {
    return DISPSTAT;
}

WRITE_CALLBACK(MMIO::WriteDISPSTAT) {
    // VBlank/HBlank/VCount flags can not be written to
    DISPSTAT = (DISPSTAT & 0x7) | (value & ~0x7);

    // VCount interrupt might happen with the newly written value
    CheckVCountMatch();
}

READ_PRECALL(MMIO::ReadVCount) {
    return VCount;
}

u16 MMIO::ReadKEYINPUT() {
    // todo: poll KEYCNT for interrupts
    return KEYINPUT;
}

WRITE_CALLBACK(MMIO::WriteIME) {
    CPU->IME = value & 1;
    if (value & 1) {
        // check if there are any interrupts
        CPU->ScheduleInterruptPoll();
    }
}
WRITE_CALLBACK(MMIO::WriteIE) {
    CPU->IE = value;
    if (value) {
        // check if there are any interrupts
        CPU->ScheduleInterruptPoll();
    }
}
WRITE_CALLBACK(MMIO::WriteIF) {
    CPU->IF &= ~value;
    if (CPU->IF) {
        // check if there are any interrupts
        CPU->ScheduleInterruptPoll();
    }

    // to make sure 8bit writes are handled properly, we must clear the IF part:
    WriteArray<u16>(Registers, static_cast<u32>(IORegister::IF), 0);
}
READ_PRECALL(MMIO::ReadIF) {
    return CPU->IF;
}