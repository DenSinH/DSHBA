#include "ARM7TDMI.h"

#ifdef TRACE_LOG
#include <iomanip>
#endif

ARM7TDMI::ARM7TDMI(s_scheduler *scheduler, Mem *memory)  {
    Scheduler = scheduler;
    Memory = memory;

    BuildARMTable();
    BuildTHUMBTable();

    InterruptPoll = (s_event) {
        .callback = &ARM7TDMI::InterruptPollEvent,
        .caller = this,
        .active = false,
    };

#ifdef TRACE_LOG
    trace.open("DSHBA.log", std::fstream::out | std::fstream::app);
#endif
}

void ARM7TDMI::SkipBIOS() {
    Registers[0] = 0x0800'0000;
    Registers[1] = 0xea;

    sp = 0x0300'7f00;

    SPLRBank[static_cast<u8>(Mode::FIQ)        & 0xf][0] = 0x0300'7f00;
    SPLRBank[static_cast<u8>(Mode::Supervisor) & 0xf][0] = 0x0300'7fe0;  // mostly here for show
    SPLRBank[static_cast<u8>(Mode::Abort)      & 0xf][0] = 0x0300'7f00;  // mostly here for show
    SPLRBank[static_cast<u8>(Mode::IRQ)        & 0xf][0] = 0x0300'7fa0;
    SPLRBank[static_cast<u8>(Mode::Undefined)  & 0xf][0] = 0x0300'7f00;  // mostly here for show

    pc = 0x0800'0000;
    CPSR = 0x6000'001f;

    this->FakePipelineFlush();
    this->pc += 4;
}

void ARM7TDMI::Step() {
    u32 instruction;

#ifdef ALWAYS_TRACE_LOG
    LogState();
#elif defined(TRACE_LOG)
    if (TraceSteps > 0) {
        TraceSteps--;
        LogState();
    }
#endif

    const bool ARMMode = !(CPSR & static_cast<u32>(CPSRFlags::T));
    if (unlikely(Pipeline.Count)) {
        // we only have stuff in the pipeline if writes near PC happened
        instruction = Pipeline.Dequeue();
    }
    else if (ARMMode) {
        // before the instruction gets executed, we are 2 instructions ahead
        instruction = Memory->Mem::Read<u32, true>(pc - 8);
    }
    else {
        // THUMB mode
        instruction = Memory->Mem::Read<u16, true>(pc - 4);
    }

    if (ARMMode) {
        // ARM mode
        if (CheckCondition(instruction >> 28)) {
            (this->*ARMInstructions[ARMHash(instruction)])(instruction);
        }

        // we handle mode changes in the BX instruction by correcting PC for it there
        // this saves us from doing another check after every instruction
        pc += 4;
    }
    else {
        // THUMB mode
        (this->*THUMBInstructions[THUMBHash((u16)instruction)])((u16)instruction);
        // same here
        pc += 2;
    }
}

void ARM7TDMI::FakePipelineFlush() {
    this->Pipeline.Clear();

    if (!(CPSR & static_cast<u32>(CPSRFlags::T))) {
        timer += Mem::GetAccessTime<u32>(static_cast<MemoryRegion>(pc >> 24)) << 1;
        // ARM mode
        this->pc += 4;
    }
    else {
        timer += Mem::GetAccessTime<u16>(static_cast<MemoryRegion>(pc >> 24)) << 1;
        // THUMB mode
        this->pc += 2;
    }
}

void ARM7TDMI::PipelineReflush() {
    this->Pipeline.Clear();
    // if instructions that should be in the pipeline get written to
    // PC is in an instruction when this happens (marked by a bool in the template)
    log_warn("Reflush for write near PC (%x)", pc);

    // don't add memory access timings for reflushes, because they should have already happened
    if (!(CPSR & static_cast<u32>(CPSRFlags::T))) {
        // ARM mode
        this->Pipeline.Enqueue(this->Memory->Mem::Read<u32, false>(this->pc - 4));
        this->Pipeline.Enqueue(this->Memory->Mem::Read<u32, false>(this->pc));
    }
    else {
        // THUMB mode
        this->Pipeline.Enqueue(this->Memory->Mem::Read<u16, false>(this->pc - 2));
        this->Pipeline.Enqueue(this->Memory->Mem::Read<u16, false>(this->pc));
    }
}

SCHEDULER_EVENT(ARM7TDMI::InterruptPollEvent) {
    auto cpu = (ARM7TDMI*)caller;

    if (cpu->IF & cpu->IE) {
        // disable halt state
        cpu->Halted = false;
        log_cpu("Interrupt requested and in IE");
        if (cpu->IME && !(cpu->CPSR & static_cast<u32>(CPSRFlags::I))) {
            log_cpu("Interrupt!");
            // actually do interrupt
            cpu->SPSRBank[static_cast<u8>(Mode::IRQ) & 0xf] = cpu->CPSR;
            cpu->ChangeMode(Mode::IRQ);
            cpu->CPSR |= static_cast<u32>(CPSRFlags::I);

            // todo: set memory BIOS readstate

            // address of instruction that did not get executed + 4
            // in THUMB mode, we are 4 bytes ahead, in ARM mode, we are 8 bytes ahead
            cpu->lr = cpu->pc - ((cpu->CPSR & static_cast<u32>(CPSRFlags::T)) ? 0 : 4);
            cpu->CPSR &= ~static_cast<u32>(CPSRFlags::T);  // enter ARM mode

            cpu->pc = static_cast<u32>(ExceptionVector::IRQ);
            cpu->FakePipelineFlush();
            cpu->pc += 4;  // get ready to receive next instruction
        }
    }
}

void ARM7TDMI::ScheduleInterruptPoll() {
    if (!InterruptPoll.active) {
        // schedule immediately
        add_event_after(Scheduler, &InterruptPoll, 0);
    }
}

#if defined(TRACE_LOG) || defined(ALWAYS_TRACE_LOG)
void ARM7TDMI::LogState() {
    if (static_cast<Mode>(CPSR & static_cast<u32>(CPSRFlags::Mode)) != Mode::Supervisor) {
        trace << std::setfill('0') << std::setw(8) << std::hex << Registers[0] << " ";
        trace << std::setfill('0') << std::setw(8) << std::hex << Registers[1] << " ";
        trace << std::setfill('0') << std::setw(8) << std::hex << Registers[2] << " ";
        trace << std::setfill('0') << std::setw(8) << std::hex << Registers[3] << " ";
        trace << std::setfill('0') << std::setw(8) << std::hex << Registers[4] << " ";
        trace << std::setfill('0') << std::setw(8) << std::hex << Registers[5] << " ";
        trace << std::setfill('0') << std::setw(8) << std::hex << Registers[6] << " ";
        trace << std::setfill('0') << std::setw(8) << std::hex << Registers[7] << " ";
        trace << std::setfill('0') << std::setw(8) << std::hex << Registers[8] << " ";
        trace << std::setfill('0') << std::setw(8) << std::hex << Registers[9] << " ";
        trace << std::setfill('0') << std::setw(8) << std::hex << Registers[10] << " ";
        trace << std::setfill('0') << std::setw(8) << std::hex << Registers[11] << " ";
        trace << std::setfill('0') << std::setw(8) << std::hex << Registers[12] << " ";
        trace << std::setfill('0') << std::setw(8) << std::hex << Registers[13] << " ";
        trace << std::setfill('0') << std::setw(8) << std::hex << Registers[14] << " ";
        trace << std::setfill('0') << std::setw(8) << std::hex << Registers[15] - ((CPSR & static_cast<u32>(CPSRFlags::T)) ? 2 : 4) << " ";
        trace << "cpsr: ";
        trace << std::setfill('0') << std::setw(8) << std::hex << CPSR << std::endl;
    }
}
#endif