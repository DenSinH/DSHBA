#include "MMIO.h"

#include "const.h"

#include "../ARM7TDMI/ARM7TDMI.h"
#include "../Mem/Mem.h"

MMIO::MMIO(GBAPPU* ppu, GBAAPU* apu, ARM7TDMI* cpu, Mem* memory, s_scheduler* scheduler) {
    PPU = ppu;
    APU = apu;
    CPU = cpu;
    Memory = memory;
    Scheduler = scheduler;

    // set to 0xff to detect HALT writes properly
    WriteArray<u8>(Registers, static_cast<u32>(IORegister::HALTCNT), 0xff);

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

    Halt = {
        .callback = HaltEvent,
        .caller   = this,
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

    Timers[0].Overflow = (s_event) {
        .callback = TimerOverflowEvent<0>,
        .caller   = this,
    };
    Timers[1].Overflow = (s_event) {
        .callback = TimerOverflowEvent<1>,
        .caller   = this,
    };
    Timers[2].Overflow = (s_event) {
        .callback = TimerOverflowEvent<2>,
        .caller   = this,
    };
    Timers[3].Overflow = (s_event) {
        .callback = TimerOverflowEvent<3>,
        .caller   = this,
    };

    Timers[0].FIFOA = &APU->fifo[0];
    Timers[0].FIFOB = &APU->fifo[1];

    Scheduler->AddEvent(&HBlank);
    Scheduler->AddEvent(&VBlank);
}

void MMIO::Reset() {
    memset(Registers, 0, sizeof(Registers));

    // we don't want to write to the interrupt control registers to not enter HALT state
    for (int i = 0; i < 0x200; i++) {
        if (WriteCallback[i >> 1]) {
            (this->*WriteCallback[i >> 1])(0);
        }
    }

    // set to 0xff to detect HALT writes properly
    WriteArray<u8>(Registers, static_cast<u32>(IORegister::HALTCNT), 0xff);

    CPU->IE  = 0;
    CPU->IF  = 0;
    CPU->IME = 0;
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

    DMAsActive++;
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
    DMAsActive--;

    // non-repeating or immediate DMAs get disabled
    if (!(control & static_cast<u16>(DMACNT_HFlags::Repeat)) ||
            (control & static_cast<u16>(DMACNT_HFlags::StartTiming)) == static_cast<u16>(DMACNT_HFlags::StartImmediate)
    ) {
        DMAData[x].CNT_H &= ~static_cast<u16>(DMACNT_HFlags::Enable);
        DMAEnabled[x] = false;
    }
    else {
        // marked as repeat
        // it is already marked as enabled

        // reload DMA data
        if (DMAData[x].Audio) {
            DMAData[x].CNT_L = 4;
        }
        else {
            DMAData[x].CNT_L = ReadArray<u16>(Registers, static_cast<u32>(IORegister::DMA0CNT_L) + x * 0xc);
        }

        if (!DMAData[x].Audio && (DMAData[x].CNT_H & static_cast<u16>(DMACNT_HFlags::DestAddrControl)) == static_cast<u16>(DMACNT_HFlags::DestIncrementReload)) {
            DMAData[x].DAD = ReadArray<u32>(Registers, static_cast<u32>(IORegister::DMA0DAD) + x * 0xc);
        }
    }

    // write back the DMA control data (with enabled clear)
    // todo: shadow this for audio DMAs?
    WriteArray<u16>(Registers, static_cast<u32>(IORegister::DMA0CNT_H) + x * 0xc, DMAData[x].CNT_H);

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

void MMIO::TriggerAudioDMA(u32 addr) {
    for (int i = 1; i <= 2; i++) {
        if (!DMAEnabled[i]) {
            continue;
        }

        if ((DMAData[i].CNT_H & static_cast<u16>(DMACNT_HFlags::StartTiming)) == static_cast<u16>(DMACNT_HFlags::StartSpecial)) {
            if (DMAData[i].DAD == addr) {
                TriggerDMAChannel(i);
            }
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
        IO->Scheduler->AddEvent(&IO->HBlankFlag);

        // clear after 226 cycles
        event->time += CYCLES_HBLANK;
        IO->Scheduler->AddEvent(event);

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
        IO->Scheduler->AddEvent(event);

        // increment VCount
        IO->VCount++;

        switch (IO->VCount) {
            case TOTAL_SCREEN_HEIGHT:
                IO->VCount = 0;

                // reset reference scanline for frame
                IO->ReferenceLine2 = 0;
                IO->ReferenceLine3 = 0;
                break;
            case (TOTAL_SCREEN_HEIGHT - 1):
                // VBlank flag is cleared in scanline 227 as opposed to scanline 0
                IO->DISPSTAT &= ~static_cast<u16>(DISPSTATFlags::VBlank);
                break;
            case 162:
                // disable video capture DMA
                if (IO->DMAEnabled[3]) {
                    if ((IO->DMAData[3].CNT_H & static_cast<u16>(DMACNT_HFlags::StartTiming)) == static_cast<u16>(DMACNT_HFlags::StartSpecial)) {
                        log_dma("Triggering video capture DMA");
                        IO->DMAData[3].CNT_H &= ~static_cast<u16>(DMACNT_HFlags::Enable);
                        IO->DMAEnabled[3] = false;
                    }
                }
            default:
                if (IO->VCount >= 2 && IO->VCount < 162) {
                    // Video capture DMA
                    if (IO->DMAEnabled[3]) {
                        if ((IO->DMAData[3].CNT_H & static_cast<u16>(DMACNT_HFlags::StartTiming)) == static_cast<u16>(DMACNT_HFlags::StartSpecial)) {
                            log_dma("Triggering video capture DMA");
                            IO->TriggerDMAChannel(3);
                        }
                    }
                }
                break;
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
    return false;  // the InterruptPoll event affects the CPU, not this one
}

SCHEDULER_EVENT(MMIO::HBlankFlagEvent) {
    auto IO = (MMIO*)caller;

    IO->DISPSTAT |= static_cast<u16>(DISPSTATFlags::HBLank);
    return false;
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
        IO->Scheduler->AddEvent(event);

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
        IO->Scheduler->AddEvent(event);
    }
    return false;  // the InterruptPoll event affects the CPU, not this one
}

WRITE_CALLBACK(MMIO::WriteDISPCNT) {
    DISPCNT = value;
    if ((ScanlineAccumLayerFlags.DISPCNT & 7) != (value & 7)) {
        ScanlineAccumLayerFlags.ModeChange = true;
    }
    ScanlineAccumLayerFlags.DISPCNT |= value;
}

READ_PRECALL(MMIO::ReadDISPSTAT) {
    return DISPSTAT;
}

WRITE_CALLBACK(MMIO::WriteDISPSTAT) {
    // VBlank/HBlank/VCount flags can not be written to
    DISPSTAT = (DISPSTAT & 0x7) | (value & ~0x7);

    // VCount interrupt might happen with the newly written value
    if ((DISPSTAT >> 8) == VCount) {
        if (!(DISPSTAT & static_cast<u16>(DISPSTATFlags::VCount))) {
            // there was no match yet
            DISPSTAT |= static_cast<u16>(DISPSTATFlags::VCount);
            if (DISPSTAT & static_cast<u16>(DISPSTATFlags::VCountIRQ)) {
                TriggerInterrupt(static_cast<u16>(Interrupt::VCount));
            }
        }
    }
    else {
        DISPSTAT &= ~static_cast<u16>(DISPSTATFlags::VCount);
    }
}

READ_PRECALL(MMIO::ReadVCount) {
    return VCount;
}

WRITE_CALLBACK(MMIO::WriteBLDCNT) {
    BLDCNT = value;
    if (ScanlineAccumLayerFlags.BLDCNT != value) {
        ScanlineAccumLayerFlags.BlendChange = true;
    }
    ScanlineAccumLayerFlags.BLDCNT |= value;
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

void MMIO::CheckKEYINPUTIRQ() {
    if (KEYCNT & static_cast<u16>(KEYCNTFlags::IRQEnable)) {
        u16 mask = KEYCNT & static_cast<u16>(KEYCNTFlags::Mask);
        if ((KEYCNT & static_cast<u16>(KEYCNTFlags::IRQCondition)) == static_cast<u16>(KEYCNTFlags::IRQAND)) {
            if ((KEYINPUT & mask) == mask) {
                TriggerInterrupt(static_cast<u16>(Interrupt::Keypad));
            }
        }
        else {  // IRQOR
            if (KEYINPUT & mask) {
                TriggerInterrupt(static_cast<u16>(Interrupt::Keypad));
            }
        }
    }
}

WRITE_CALLBACK(MMIO::WriteNoiseCNT_L) {
    APU->ns.LengthCounter = (64 - value & 0x001f);
    APU->ns.EnvelopeTime = (value >> 8) & 7;
    APU->ns.EnvelopeUp   = (value & 0x0800) != 0;

    bool was_off = APU->ns.Volume == 0;
    APU->ns.Volume = value >> 12;

    // might change event state if volume was turned on/off
    if (was_off ^ (APU->ns.Volume == 0)) {
        APU->ns.UpdateEvent();
    }
}

WRITE_CALLBACK(MMIO::WriteNoiseCNT_H) {
    u32 r = value & 0x0007;
    u32 s = (value & 0x00f0) >> 4;
    // ARM7TDMI.Frequency / 524288 = 32
    if (r == 0)
    {
        // interpret as 0.5 instead
        APU->ns.SetPeriod(32 * 2 * (2 << s));
    }
    else
    {
        APU->ns.SetPeriod(32 * (r * (2 << s)));
    }

    APU->ns.CounterStepWidth = (value & 0x0008) != 0;
    APU->ns.LengthFlag = (value & 0x4000) > 0;
    if (value & 0x8000)  {
        APU->ns.Trigger();
    }
    else {
        // event might still change because of period change
        APU->ns.UpdateEvent();
    }
}

WRITE_CALLBACK(MMIO::WriteWaveCNT_L) {
    APU->wav.DoubleBanked = (value & 0x0020) != 0;
    APU->wav.SwitchBanks((value & 0x0040) >> 6);

    bool old_playback = APU->wav.PlayBack;
    APU->wav.PlayBack = (value & 0x0080) != 0;

    if (old_playback ^ APU->wav.PlayBack) {
        APU->wav.UpdateEvent();
    }
}

WRITE_CALLBACK(MMIO::WriteWaveCNT_H) {
    APU->wav.LengthCounter = 256 - (value & 0xff);
    APU->wav.SetVolume((value & 0x6000) >> 13);
    APU->wav.ForceVolume = (value & 0x8000) != 0;

    APU->wav.UpdateEvent();
}

WRITE_CALLBACK(MMIO::WriteWaveCNT_X) {
    APU->wav.SetPeriod((2048 - (value & 0x07ff)) << 3);
    APU->wav.LengthFlag = (value & 0x4000) != 0;
    if (value & 0x8000) {
        APU->wav.Trigger();
    }
    else {
        APU->wav.UpdateEvent();
    }
}

WRITE_CALLBACK(MMIO::WriteSOUNDCNT_L) {
    APU->MasterVolumeRight =  value        & 0x7;
    APU->MasterVolumeLeft  = (value >> 4)  & 0x7;
    APU->SoundEnableRight  = (value >> 8)  & 0xf;
    APU->SoundEnableLeft   = (value >> 12) & 0xf;
}

WRITE_CALLBACK(MMIO::WriteSOUNDCNT_H) {
    APU->SOUNDCNT_H = value;

    if (value & static_cast<u16>(SOUNDCNT_HFlags::DMAATMSelect)) {
        // timer 1 selected for FIFO A
        Timers[1].FIFOA = &APU->fifo[0];
        Timers[0].FIFOA = nullptr;
    }
    else {
        // timer 0 selected
        Timers[1].FIFOA = nullptr;
        Timers[0].FIFOA = &APU->fifo[0];
    }

    // same for timer B
    if (value & static_cast<u16>(SOUNDCNT_HFlags::DMABTMSelect)) {
        Timers[1].FIFOB = &APU->fifo[1];
        Timers[0].FIFOB = nullptr;
    }
    else {
        Timers[1].FIFOB = nullptr;
        Timers[0].FIFOB = &APU->fifo[1];
    }
}

WRITE_CALLBACK(MMIO::WriteSOUNDCNT_X) {
    APU->SOUNDCNT_X = value;
}

READ_PRECALL(MMIO::ReadKEYINPUT) {
    CheckKEYINPUTIRQ();
    return KEYINPUT;
}

WRITE_CALLBACK(MMIO::WriteKEYCNT) {
    KEYCNT = value;
    CheckKEYINPUTIRQ();
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

SCHEDULER_EVENT(MMIO::HaltEvent) {
    auto IO = (MMIO*)caller;

    while (IO->CPU->Halted) {
        *IO->Scheduler->timer = IO->Scheduler->PeekEvent();
        IO->Scheduler->DoEvents();
    }
    return false;  // the InterruptPoll event affects the CPU, not this one
}

WRITE_CALLBACK(MMIO::WritePOSTFLG_HALTCNT) {
#ifndef SINGLE_CPI
    if (!(value & 0xff00)) {
        CPU->Halted = true;

        // halting needs to happen outside of the instruction scope
        Scheduler->AddEventAfter(&Halt, 0);

        // reset to 0xff to detect HALT writes properly
        WriteArray<u8>(Registers, static_cast<u32>(IORegister::HALTCNT), 0xff);
    }
#endif
}