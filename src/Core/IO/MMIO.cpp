#include "MMIO.h"

#include "Interrupts.h"

#include "const.h"

#include "../ARM7TDMI/ARM7TDMI.h"
#include "../Mem/Mem.h"

// needed for frame size data:
#include "../PPU/shaders/GX_constants.h"

MMIO::MMIO(GBAPPU* ppu, ARM7TDMI* cpu, Mem* memory, s_scheduler* scheduler) {
    PPU = ppu;
    CPU = cpu;
    Memory = memory;
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

    DMAData[0].CNT_L_MAX = 0x4000;
    DMAData[1].CNT_L_MAX = 0x4000;
    DMAData[2].CNT_L_MAX = 0x4000;
    DMAData[3].CNT_L_MAX = 0x10000;

    add_event(scheduler, &HBlankFlag);
    add_event(scheduler, &VBlankFlag);
}

void MMIO::TriggerDMA(u32 x) {
    const u16 control = DMAData[x].CNT_H;

    bool other_dma_active = false;
    for (int i = 0; i < 4; i++) {
        if (i == x) {
            continue;
        }

        if (DMAEnabled[i]) {
            other_dma_active = true;
            break;
        }
    }

    if (control & static_cast<u16>(DMACNT_HFlags::WordSized)) {
        if (other_dma_active) {
            Memory->DoDMA<u32, true>(&DMAData[x]);
        }
        else {
            Memory->DoDMA<u32, false>(&DMAData[x]);
        }
    }
    else {
        if (other_dma_active) {
            Memory->DoDMA<u16, true>(&DMAData[x]);
        }
        else {
            Memory->DoDMA<u16, false>(&DMAData[x]);
        }
    }

    // non-repeating or immediate DMAs get disabled
    if (!(control & static_cast<u16>(DMACNT_HFlags::Repeat)) ||
            (control & static_cast<u16>(DMACNT_HFlags::StartTiming)) != static_cast<u16>(DMACNT_HFlags::StartImmediate)
    ) {
        DMAData[x].CNT_H &= ~static_cast<u16>(DMACNT_HFlags::Enable);
        DMAEnabled[x] = false;
    }
    // otherwise, it is already marked as enabled

    // write back the DMA data
#ifdef DIRECT_DMA_DATA_COPY
    memcpy(
            &Registers[static_cast<u32>(IORegister::DMA0SAD) + x * 0xc],
            &DMAData[x],
            sizeof(s_DMAData)
    );
#else
    WriteArray<u32>(Registers, static_cast<u32>(IORegister::DMA0SAD)   + x * 0xc, DMAData[x].SAD);
    WriteArray<u32>(Registers, static_cast<u32>(IORegister::DMA0DAD)   + x * 0xc, DMAData[x].DAD);
    WriteArray<u32>(Registers, static_cast<u32>(IORegister::DMA0CNT_L) + x * 0xc, DMAData[x].CNT_L);
    WriteArray<u32>(Registers, static_cast<u32>(IORegister::DMA0CNT_H) + x * 0xc, DMAData[x].CNT_H);
#endif

    // trigger IRQ
    if (control & static_cast<u16>(DMACNT_HFlags::IRQ)) {
        CPU->IF |= static_cast<u16>(Interrupt::DMA0) << x;
        CPU->ScheduleInterruptPoll();
    }
}

SCHEDULER_EVENT(MMIO::HBlankFlagEvent) {
    /*
     * HBlank IRQ is requested after 960 cycles <- todo
     * HBlank flag is clear for 1006 cycles
     * Then for the rest of the scanline (226 cycles), HBlank flag is set
     * */
    auto IO = (MMIO*)caller;

    IO->DISPSTAT ^= static_cast<u16>(DISPSTATFlags::HBLank);
    if (IO->DISPSTAT & static_cast<u16>(DISPSTATFlags::HBLank)) {
        // HBlank was set, clear after 226 cycles
        event->time += CYCLES_HBLANK_SET;

        if (IO->VCount < VISIBLE_SCREEN_HEIGHT) {
            IO->PPU->BufferScanline(IO->VCount);
        }

        // HBlank interrupts
        if (IO->DISPSTAT & static_cast<u16>(DISPSTATFlags::HBLankIRQ)) {
            IO->CPU->IF |= static_cast<u16>(Interrupt::HBlank);
            IO->CPU->ScheduleInterruptPoll();
        }

        // HBlank DMAs
        for (int i = 0; i < 4; i++) {
            if (!IO->DMAEnabled[i]) {
                continue;
            }

            if ((IO->DMAData[i].CNT_H & static_cast<u16>(DMACNT_HFlags::StartTiming)) == static_cast<u16>(DMACNT_HFlags::StartHBlank)) {
                IO->TriggerDMA(i);
            }
        }
    }
    else {
        // HBlank was cleared, set after 1006 cycles
        event->time += CYCLES_HBLANK_CLEAR;

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

        // VBlank DMAs
        for (int i = 0; i < 4; i++) {
            if (!IO->DMAEnabled[i]) {
                continue;
            }

            if ((IO->DMAData[i].CNT_H & static_cast<u16>(DMACNT_HFlags::StartTiming)) == static_cast<u16>(DMACNT_HFlags::StartVBlank)) {
                IO->TriggerDMA(i);
            }
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