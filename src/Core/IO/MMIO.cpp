#include "MMIO.h"

#include "const.h"

#include "../ARM7TDMI/ARM7TDMI.h"
#include "../Mem/Mem.h"

MMIO::MMIO(GBAPPU* ppu, ARM7TDMI* cpu, Mem* memory, s_scheduler* scheduler) {
    PPU = ppu;
    CPU = cpu;
    Memory = memory;
    Scheduler = scheduler;

    HBlank = {
        .callback = HBlankEvent,
        .caller   = this,
        .time = 960 // (see below)
    };

    HBlankFlag = {
        .callback = HBlankFlagEvent,
        .caller   = this,
        // time is set by HBlank event
    };

    VBlank = {
        .callback = VBlankEvent,
        .caller   = this,
        .time     = CYCLES_PER_SCANLINE * VISIBLE_SCREEN_HEIGHT
    };

    DMAData[0].CNT_L_MAX = 0x4000;
    DMAData[1].CNT_L_MAX = 0x4000;
    DMAData[2].CNT_L_MAX = 0x4000;
    DMAData[3].CNT_L_MAX = 0x10000;

    DMAStart[0] = (s_event) {
        .callback = DMAStartEvent<0>,
        .caller = this,
    };
    DMAStart[1] = (s_event) {
        .callback = DMAStartEvent<1>,
        .caller = this,
    };
    DMAStart[2] = (s_event) {
        .callback = DMAStartEvent<2>,
        .caller = this,
    };
    DMAStart[3] = (s_event) {
        .callback = DMAStartEvent<3>,
        .caller = this,
    };

    Timer[0].Overflow = (s_event) {
        .callback = TimerOverflowEvent<0>,
        .caller   = this,
    };
    Timer[1].Overflow = (s_event) {
        .callback = TimerOverflowEvent<1>,
        .caller   = this,
    };
    Timer[2].Overflow = (s_event) {
        .callback = TimerOverflowEvent<2>,
        .caller   = this,
    };
    Timer[3].Overflow = (s_event) {
        .callback = TimerOverflowEvent<3>,
        .caller   = this,
    };

    add_event(scheduler, &HBlank);
    add_event(scheduler, &VBlank);
}

void MMIO::TriggerInterrupt(u16 interrupt) {
    CPU->IF |= interrupt;
    CPU->ScheduleInterruptPoll();
}

void MMIO::RunDMAChannel(u8 x) {
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
            (control & static_cast<u16>(DMACNT_HFlags::StartTiming)) == static_cast<u16>(DMACNT_HFlags::StartImmediate)
    ) {
        DMAData[x].CNT_H &= ~static_cast<u16>(DMACNT_HFlags::Enable);
        DMAEnabled[x] = false;
    }
    // otherwise, it is already marked as enabled

    // write back the DMA data
#ifdef DIRECT_IO_DATA_COPY
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
        log_dma("Trigger DMA%x interrupt (IE %x)", x, CPU->IE);
        TriggerInterrupt(static_cast<u16>(Interrupt::DMA0) << x);
    }
}

void MMIO::TriggerDMATiming(DMACNT_HFlags start_timing) {
    for (int i = 0; i < 4; i++) {
        if (!DMAEnabled[i]) {
            continue;
        }

        if ((DMAData[i].CNT_H & static_cast<u16>(DMACNT_HFlags::StartTiming)) == static_cast<u16>(start_timing)) {
            log_dma("Triggering DMA %x start timing %x", i, static_cast<u16>(start_timing));
            TriggerDMAChannel(i);
        }
    }
}

SCHEDULER_EVENT(MMIO::HBlankEvent) {
    /*
     * HBlank IRQ is requested after 960 cycles
     * HBlank flag is clear for 1006 cycles
     * Then for the rest of the scanline (226 cycles), HBlank flag is set
     * */
    auto IO = (MMIO*)caller;

    if (!(IO->DISPSTAT & static_cast<u16>(DISPSTATFlags::HBLank))) {
        // HBlank, set flag after 46 cycles
        IO->HBlankFlag.time = event->time + CYCLES_HBLANK_FLAG_DELAY;
        add_event(scheduler, &IO->HBlankFlag);

        // clear after 226 cycles
        event->time += CYCLES_HBLANK;
        add_event(scheduler, event);

        // buffer scanline & HBlank DMAs
        if (IO->VCount < VISIBLE_SCREEN_HEIGHT) {
            IO->PPU->BufferScanline(IO->VCount);
            IO->TriggerDMATiming(DMACNT_HFlags::StartHBlank);
        }

        // HBlank interrupts
        if (IO->DISPSTAT & static_cast<u16>(DISPSTATFlags::HBLankIRQ)) {
            IO->TriggerInterrupt(static_cast<u16>(Interrupt::HBlank));
        }
    }
    else {
        // HBlank was cleared, set after 1006 cycles
        IO->DISPSTAT &= ~static_cast<u16>(DISPSTATFlags::HBLank);
        event->time += CYCLES_HDRAW;
        add_event(scheduler, event);

        // increment VCount
        IO->VCount++;

        if (unlikely(IO->VCount == TOTAL_SCREEN_HEIGHT)) {
            IO->VCount = 0;

            // reset reference scanline for frame
            IO->ReferenceLine2 = 0;
            IO->ReferenceLine3 = 0;
        }
        else if (unlikely(IO->VCount == 1)) {
            // Video capture DMA
            if (IO->DMAEnabled[3]) {
                if ((IO->DMAData[3].CNT_H & static_cast<u16>(DMACNT_HFlags::StartTiming)) == static_cast<u16>(DMACNT_HFlags::StartSpecial)) {
                    // todo: stop DMA at scanline 162
                    log_dma("Triggering video capture DMA");
                    IO->TriggerDMAChannel(3);
                }
            }
        }
        else if (unlikely(IO->VCount == (TOTAL_SCREEN_HEIGHT - 1))) {
            // VBlank flag is cleared in scanline 227 as opposed to scanline 0
            IO->DISPSTAT &= ~static_cast<u16>(DISPSTATFlags::VBlank);
        }

        if (IO->VCount == IO->DISPSTAT >> 8) {
            // VCount match
            IO->DISPSTAT |= static_cast<u16>(DISPSTATFlags::VCount);
            if (IO->DISPSTAT & static_cast<u16>(DISPSTATFlags::VCountIRQ)) {
                IO->TriggerInterrupt(static_cast<u16>(Interrupt::VCount));
            }
        }
        else {
            IO->DISPSTAT &= ~static_cast<u16>(DISPSTATFlags::VCount);
        }
    }
}

SCHEDULER_EVENT(MMIO::HBlankFlagEvent) {
    auto IO = (MMIO*)caller;

    IO->DISPSTAT |= static_cast<u16>(DISPSTATFlags::HBLank);
}

SCHEDULER_EVENT(MMIO::VBlankEvent) {
    /*
     * VBlank is set after 160 scanlines, set for the rest of the frame
     * */
    auto IO = (MMIO*)caller;

    IO->LCDVBlank ^= true;
    if (IO->LCDVBlank) {
        IO->DISPSTAT |= static_cast<u16>(DISPSTATFlags::VBlank);
        // VBlank was set, clear after 68 scanlines (total frame height - visible frame height)
        event->time += CYCLES_PER_SCANLINE * (TOTAL_SCREEN_HEIGHT - VISIBLE_SCREEN_HEIGHT);
        add_event(scheduler, event);

        // VBlank interrupts
        if (IO->DISPSTAT & static_cast<u16>(DISPSTATFlags::VBlankIRQ)) {
            IO->TriggerInterrupt(static_cast<u16>(Interrupt::VBlank));
        }

        // VBlank DMAs
        IO->TriggerDMATiming(DMACNT_HFlags::StartVBlank);
    }
    else {
        // no longer in VBlank, set again after visible frame
        event->time += CYCLES_PER_SCANLINE * VISIBLE_SCREEN_HEIGHT;
        add_event(scheduler, event);
    }
}

READ_PRECALL(MMIO::ReadDISPSTAT) {
    return DISPSTAT;
}

WRITE_CALLBACK(MMIO::WriteDISPSTAT) {
    // VBlank/HBlank/VCount flags can not be written to
    DISPSTAT = (DISPSTAT & 0x7) | (value & ~0x7);

    // VCount interrupt might happen with the newly written value
    if (DISPSTAT & static_cast<u16>(DISPSTATFlags::VCountIRQ)) {
        if ((DISPSTAT >> 8) == VCount) {
            TriggerInterrupt(static_cast<u16>(Interrupt::VCount));
        }
    }
}

READ_PRECALL(MMIO::ReadVCount) {
    return VCount;
}

WRITE_CALLBACK(MMIO::WriteSIOCNT) {
    if (value & 0x80) {
        // started, clear start bit:
        Registers[static_cast<u32>(IORegister::SIOCNT)] &= ~0x80;

        if (value & 0x4000) {
            // IRQ enabled
            TriggerInterrupt(static_cast<u16>(Interrupt::SIO));
        }
    }
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

WRITE_CALLBACK(MMIO::WritePOSTFLG_HALTCNT) {
    if (value & 0xff00) {
        CPU->Halted = true;
        while (CPU->Halted) {
            CPU->timer = peek_event(Scheduler);
            do_events(Scheduler);
        }
    }
}