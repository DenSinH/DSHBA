#include "ARM7TDMI.h"
#include "sleeping.h"

#if defined(TRACE_LOG) || defined(ALWAYS_TRACE_LOG)
#include <iomanip>
#endif

ARM7TDMI::ARM7TDMI(s_scheduler *scheduler, Mem *memory)  {
    Scheduler = scheduler;
    timer = scheduler->timer;
    Memory = memory;

    BuildARMTable();
    BuildTHUMBTable();

    InterruptPoll = (s_event) {
        .callback = &ARM7TDMI::InterruptPollEvent,
        .caller = this,
        .active = false,
    };

#if defined(TRACE_LOG) || defined(ALWAYS_TRACE_LOG)
    trace.open("DSHBA.log", std::fstream::out | std::fstream::app);
#endif

    // if BIOS is not skipped, we need to start pc at 8 to start fetching instructions properly
    pc += 8;

#ifdef DO_BREAKPOINTS
    Breakpoints = {};
    Paused      = false;

//    add_breakpoint(&Breakpoints, 0x0800'0370);
//    add_breakpoint(&Breakpoints, 0x0800'037a);
//    add_breakpoint(&Breakpoints, 0x0800'0928);
//    add_breakpoint(&Breakpoints, 0x0800'8e74);
//    add_breakpoint(&Breakpoints, 0x0800'8e78);
#endif
}

void ARM7TDMI::Reset() {
    memset(Registers, 0, sizeof(Registers));
    memset(SPSRBank, 0, sizeof(SPSRBank));
    memset(SPLRBank, 0, sizeof(SPLRBank));
    memset(FIQBank, 0, sizeof(FIQBank));
    CPSR = 0;
    ARMMode = true;
    SPSR = 0;

    IME = 0;
    IE = 0;
    IF = 0;
    Halted = false;
    Pipeline.Clear();
    SkipBIOS();

    // reset instruction caches
    for (auto & i : ROMCache) {
        i = nullptr;
    }
    for (auto & i : iWRAMCache) {
        i = nullptr;
    }
    // pc += 8;
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

    ARMMode = true;
    this->FakePipelineFlush<true>();
    this->pc += 4;
}

void ARM7TDMI::PipelineReflush() {
    this->Pipeline.Clear();
    // if instructions that should be in the pipeline get written to
    // PC is in an instruction when this happens (marked by a bool in the template)

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
            if (cpu->CPSR & static_cast<u32>(CPSRFlags::T)) {
                // THUMB mode, flush NZ flags
                cpu->FlushNZ();
            }

            cpu->SPSRBank[static_cast<u8>(Mode::IRQ) & 0xf] = cpu->CPSR;
            cpu->ChangeMode(Mode::IRQ);
            cpu->CPSR |= static_cast<u32>(CPSRFlags::I);

            cpu->Memory->CurrentBIOSReadState = BIOSReadState::DuringIRQ;

            // address of instruction that did not get executed + 4
            // in THUMB mode, we are 4 bytes ahead, in ARM mode, we are 8 bytes ahead
            cpu->lr = cpu->pc - ((cpu->CPSR & static_cast<u32>(CPSRFlags::T)) ? 0 : 4);

            if (cpu->CPSR & static_cast<u32>(CPSRFlags::T)) {
                // THUMB mode, enter ARM mode, flush NZ flags
                cpu->CPSR &= ~static_cast<u32>(CPSRFlags::T);
                cpu->ARMMode = true;
            }

            cpu->pc = static_cast<u32>(ExceptionVector::IRQ);
            cpu->FakePipelineFlush<true>();
            cpu->pc += 4;  // get ready to receive next instruction
        }
    }
}

void ARM7TDMI::ScheduleInterruptPoll() {
    if (!InterruptPoll.active) {
        // schedule immediately
        Scheduler->AddEventAfter(&InterruptPoll, 0);
    }
}

void ARM7TDMI::iWRAMWrite(u32 address) {
    // clear all instruction caches in a CacheBlockSizeBytes sized region
    const u32 base = (((address & 0x7fff) >> 1) & ~(CacheBlockSizeBytes - 1));
    for (u32 offs = 0; offs < CacheBlockSizeBytes >> 1; offs++) {
        iWRAMCache[base + offs] = nullptr;
    }

    if ((corrected_pc & ~(CacheBlockSizeBytes - 1)) == (address & ~(CacheBlockSizeBytes - 1))) {
        // current block destroyed
        if (CurrentCache) {
            *CurrentCache = nullptr;
            CurrentCache = nullptr;
        }
    }
}

#ifdef DO_DEBUGGER
void ARM7TDMI::DebugChecks(void** const interaction) {
#ifdef ALWAYS_TRACE_LOG
    LogState();
#elif defined(TRACE_LOG)
    if (TraceSteps > 0) {
        TraceSteps--;
        LogState();
    }
#endif

#ifdef DO_BREAKPOINTS
    if (check_breakpoints(&Breakpoints, corrected_pc)) {
        log_debug("Hit breakpoint %08x", corrected_pc);
        Paused = true;
    }
#endif

    while (Paused && (Stepcount == 0) && !(*interaction)) {
        // sleep for a bit to not have a busy wait. This is for debugging anyway, and we are frozen
        // so it's okay if it's not instant
        sleep_ms(16);
    }

    if (Stepcount > 0) {
        Stepcount--;
    }
}
#endif

#ifdef DO_DEBUGGER
void ARM7TDMI::RunMakeCache(void** const until) {
#else
void ARM7TDMI::RunMakeCache() {
#endif
    // log_debug("Making cache block at %x", pc);
    while (true) {
#ifdef DO_DEBUGGER
        DebugChecks(until);
#endif
        if (unlikely(Step<true>())) {
            return;
        }

        if (unlikely(Scheduler->ShouldDoEvents())) {
            Scheduler->DoEvents();
            return;
        }

        if (unlikely(!CurrentCache)) {
            // cache destroyed by near write
            return;
        }
    }
}

#ifdef DO_DEBUGGER
void ARM7TDMI::RunCache(void** const until) {
#else
void ARM7TDMI::RunCache() {
#endif
    // log_debug("Running cache block at %x", pc);
    const i32 cycles = (*CurrentCache)->AccessTime;

    if ((*CurrentCache)->ARM) {
        // ARM mode, we need to check the condition now too
        for (auto& instr : (*CurrentCache)->Instructions) {
#ifdef DO_DEBUGGER
            DebugChecks(until);
#endif
            if (CheckCondition(instr.Instruction >> 28)) {
                (this->*instr.Pointer)(instr.Instruction);
            }

            *timer += cycles;
            pc += 4;
            if (unlikely(Scheduler->ShouldDoEvents())) {
                Scheduler->DoEvents();
                return;
            }

            // block was destroyed (very unlikely)
            if (unlikely(!CurrentCache)) {
                return;
            }
        }
    }
    else {
        // THUMB mode, no need to check instructions
        for (auto& instr : (*CurrentCache)->Instructions) {
#ifdef DO_DEBUGGER
            DebugChecks(until);
#endif
            (this->*instr.Pointer)(instr.Instruction);

            *timer += cycles;
            pc += 2;

            if (unlikely(Scheduler->ShouldDoEvents())) {
                Scheduler->DoEvents();
                return;
            }

            // block was destroyed (very unlikely)
            if (unlikely(!CurrentCache)) {
                return;
            }
        }
    }
}

void ARM7TDMI::Run(void** const until) {
    while (!(*until)) {
        CurrentCache = GetCache(corrected_pc);
        // log_debug("got cache %p at %x", CurrentCache, corrected_pc);

        if (unlikely(!CurrentCache)) {
            // nullptr: no cache (not iWRAM / ROM)
            while (true) {
                if (unlikely(Scheduler->ShouldDoEvents())) {
                    Scheduler->DoEvents();
                }

#ifdef DO_DEBUGGER
                DebugChecks(until);
#endif
                if (Step<false>()) {
                    break;
                }
            }
        }
        else if (unlikely(!(*CurrentCache))) {
            // possible cache, but none present
            // make new one
            if (ARMMode) {
                const auto access_time = Memory->GetAccessTime<u32>(static_cast<MemoryRegion>(pc >> 24)) << 1;
                *CurrentCache = std::make_unique<InstructionCache>(access_time, true);
            }
            else {
                const auto access_time = Memory->GetAccessTime<u16>(static_cast<MemoryRegion>(pc >> 24)) << 1;
                *CurrentCache = std::make_unique<InstructionCache>(access_time, false);
            }

#ifdef DO_DEBUGGER
            RunMakeCache(until);
#else
            RunMakeCache();
#endif
        }
        else {
#ifdef DO_DEBUGGER
            RunCache(until);
#else
            RunCache();
#endif
        }
    }
}

#if defined(TRACE_LOG) || defined(ALWAYS_TRACE_LOG)
void ARM7TDMI::LogState() {
    if (pc >= 0x0100'0000 && (static_cast<Mode>(CPSR & static_cast<u32>(CPSRFlags::Mode)) != Mode::Supervisor)) {
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