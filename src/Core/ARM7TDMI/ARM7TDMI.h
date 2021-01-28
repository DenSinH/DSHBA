#pragma once

#include "Pipeline.h"

#include "../Mem/Mem.h"

#include "../Scheduler/scheduler.h"
#include "../Breakpoints/breakpoints.h"

#include "default.h"
#include "helpers.h"
#include "flags.h"
#include "log.h"

#include <vector>
#include <array>
#include <memory>

#if defined(TRACE_LOG) || defined(ALWAYS_TRACE_LOG)
#include <fstream>
#endif

class ARM7TDMI;

#define sp Registers[13]
#define lr Registers[14]
#define pc Registers[15]
#define corrected_pc (pc - (!ARMMode ? 4 : 8))

constexpr size_t ARMInstructionTableSize   = 0x1000;
constexpr size_t THUMBInstructionTableSize = 0x400;

enum class CPSRFlags : u32 {
    N    = 0x8000'0000,
    Z    = 0x4000'0000,
    C    = 0x2000'0000,
    V    = 0x1000'0000,
    I    = 0x0000'0080,
    F    = 0x0000'0040,
    T    = 0x0000'0020,
    Mode = 0x0000'001f,
};

enum class State : u8 {
    ARM   = 0,
    THUMB = 1,
};

enum class Mode : u8 {
    User        = 0b10000,
    FIQ         = 0b10001,
    IRQ         = 0b10010,
    Supervisor  = 0b10011,
    Abort       = 0b10111,
    Undefined   = 0b11011,
    System      = 0b11111,
};

enum class ExceptionVector : u32 {
    Reset = 0x0000'0000,
    SWI   = 0x0000'0008,
    IRQ   = 0x0000'0018,
};

#ifdef _WIN32

#define INSTRUCTION(_name) void __fastcall (_name)(u32 instruction)

// #elif defined(__GNUC__) && __GNUC_MINOR__ > 3

#else

#define INSTRUCTION(_name) void __attribute__((fastcall)) (_name)(u32 instruction)

#endif

typedef INSTRUCTION((ARM7TDMI::*ARMInstructionPtr));
typedef INSTRUCTION((ARM7TDMI::*THUMBInstructionPtr));
typedef INSTRUCTION((ARM7TDMI::*InstructionPtr));

template<u16 instruction> static constexpr THUMBInstructionPtr GetTHUMBInstruction();
template<u32 instruction> static constexpr ARMInstructionPtr GetARMInstruction();

static constexpr ALWAYS_INLINE u32 ARMHash(const u32 instruction) {
    return ((instruction & 0x0ff0'0000) >> 16) | ((instruction & 0x00f0) >> 4);
}

static constexpr ALWAYS_INLINE u32 THUMBHash(const u16 instruction) {
    return ((instruction & 0xffc0) >> 6);
}

class ARM7TDMI {
public:
    i32* timer;

    ARM7TDMI(s_scheduler* scheduler, Mem* memory);
    ~ARM7TDMI() {

#if defined(TRACE_LOG) || defined(ALWAYS_TRACE_LOG)
        trace.close();
#endif

    };

    bool Paused = false;
#ifdef DO_DEBUGGER
    u32 Stepcount = 0;

# ifdef DO_BREAKPOINTS
    s_breakpoints Breakpoints = {};
# endif

    void DebugChecks(void** const interaction);
#endif

    void SkipBIOS();

    template<bool MakeCache>
    ALWAYS_INLINE bool Step();
    void Run(void** const until);
#ifdef DO_DEBUGGER
    void RunMakeCache(void** const until);
    void RunCache(void** const until);
#else
    void RunMakeCache();
    void RunCache();
#endif
    void PipelineReflush();
    void Reset();

private:
    friend class MMIO;  // IO needs control over the CPU
    friend class GBA;   // mostly for debugging
    friend class Initializer;

    // this is to hack CLion into thinking we can access everything
    friend class ARM7TDMI_INL;

#ifdef BENCHMARKING
    friend void benchmark();
#endif

#if defined(TRACE_LOG) || defined(ALWAYS_TRACE_LOG)
    std::fstream trace;
    void LogState();
#ifdef TRACE_LOG
    u32 TraceSteps = 0;
#endif

#endif

    u32 CPSR            = {};
    u32 SPSR            = {};
    u32 SPSRBank[16]    = {};
    u32 SPLRBank[16][2] = {};
    u32 FIQBank[2][5]   = {};  // store user mode registers into 0, FIQ registers into 1
    u32 Registers[16]   = {};

    bool ARMMode = true;

    struct CachedInstruction {
        CachedInstruction(u32 instruction, InstructionPtr ptr) :
                Instruction(instruction),
                Pointer(ptr) {

        }

        const u32 Instruction;
        const InstructionPtr Pointer;
    };

    struct InstructionCache {
        InstructionCache(u16 access_time, bool arm) :
                AccessTime(access_time),
                ARM(arm) {
            Instructions.reserve(Mem::InstructionCacheBlockSizeBytes >> 1);
        }

        const bool ARM;
        const u16 AccessTime;
        std::vector<CachedInstruction> Instructions;
    };

    // shift by 1 because of instruction alignment
    std::array<std::unique_ptr<InstructionCache>, (0x4000 >> 1)> BIOSCache = {};
    // we don't want to include the stack so that we don't have to check this all the time
    std::array<std::vector<u32>, (0x8000 - Mem::StackSize) / Mem::InstructionCacheBlockSizeBytes> iWRAMCacheFilled = {};
    std::array<std::unique_ptr<InstructionCache>, ((0x8000 - Mem::StackSize) >> 1)> iWRAMCache = {};
    std::array<std::unique_ptr<InstructionCache>, (0x0200'0000 >> 1)> ROMCache = {};
    
    std::unique_ptr<InstructionCache>* CurrentCache = nullptr;

    static constexpr bool InCacheRegion(const u32 address) {
        const bool in_bios = address < 0x4000;
        const bool in_iwram = (static_cast<MemoryRegion>(address >> 24) == MemoryRegion::iWRAM) && ((address & 0x7fff) < (0x8000 - Mem::StackSize));
        const bool in_rom = address >= (static_cast<u32>(MemoryRegion::ROM_L) << 24);
        return in_iwram || in_rom || in_bios;
    }

    constexpr std::unique_ptr<InstructionCache>* GetCache(const u32 address) {
        switch (static_cast<MemoryRegion>(address >> 24)) {
            case MemoryRegion::BIOS: {
                const u32 index = (address & 0x3fff) >> 1;
                if (likely(address < 0x4000)) {
                    if (BIOSCache[index]) {
                        return &BIOSCache[index];
                    }
                    BIOSCache[index] = nullptr;
                    return &BIOSCache[index];
                }
                return nullptr;
            }
            case MemoryRegion::iWRAM: {
                const u32 index = (address & 0x7fff) >> 1;
                if ((address & 0x7fff) < (0x8000 - Mem::StackSize)) {
                    if (iWRAMCache[index]) {
                        return &iWRAMCache[index];
                    }
                    // mark index as filled
                    iWRAMCacheFilled[(address & 0x7fff) / Mem::InstructionCacheBlockSizeBytes].push_back(index);
                    iWRAMCache[index] = nullptr;
                    return &iWRAMCache[index];
                }
                return nullptr;
            }
            case MemoryRegion::ROM_L:
            case MemoryRegion::ROM_H:
            case MemoryRegion::ROM_L1:
            case MemoryRegion::ROM_H1:
            case MemoryRegion::ROM_L2:
            case MemoryRegion::ROM_H2: {
                const u32 index = (address & 0x1ff'ffff) >> 1;
                if (ROMCache[index]) {
                    return &ROMCache[index];
                }
                ROMCache[index] = nullptr;
                return &ROMCache[index];
            }
            default:
                return nullptr;
        }
    }
    void iWRAMWrite(u32 address);


    // used to reduce the amount of times we need to set the NZ flags in THUMB mode
    // basically, we only need to update them when transitioning back to ARM or on conditional branches
    /*
     * Transitions back to ARM:
     *  - IRQ
     *  - SWI
     *  - BX
     * */
    u64 LastNZ;
    static constexpr const u64 NZDefault[4] {
        /* ~N, ~Z */ 1,
        /* ~N, Z */  0,
        /* N, ~Z */  0x8000'0000,
        /* N, Z */   0x1'0000'0000,  // impossible to encode in these bits
    };

    ALWAYS_INLINE void SetLastNZ() {
        LastNZ = NZDefault[CPSR >> 30];
    }

    ALWAYS_INLINE void FlushNZ() {
        if (unlikely(LastNZ & 0x1'0000'0000)) {
            // value wasn't affected in THUMB mode, has to have been NZ
            CPSR |= static_cast<u32>(CPSRFlags::N) | static_cast<u32>(CPSRFlags::Z);
        }
        else {
            SetNZ(LastNZ);
        }
    }

    // (Conditions[cond_field] >> CPSRFlags) & 1 is truth value
    static constexpr const u16 Conditions[0x10] = {
            /* 0 */ 0xf0f0,
            /* 1 */ 0x0f0f,
            /* 2 */ 0xcccc,
            /* 3 */ 0x3333,
            /* 4 */ 0xff00,
            /* 5 */ 0x00ff,
            /* 6 */ 0xaaaa,
            /* 7 */ 0x5555,
            /* 8 */ 0x0c0c,
            /* 9 */ 0xf3f3,
            /* a */ 0xaa55,
            /* b */ 0x55aa,
            /* c */ 0x0a05,
            /* d */ 0xf5fa,
            /* e */ 0xffff,
            /* f */ 0x0000,
    };

    // Manipulated by MMIO
    // the CPU should be able to check for interrupts on its own though, so we need to keep these in here
    // mostly, interrupts will be raised through MMIO
    u16 IME      = 0;
    u16 IF       = 0;
    u16 IE       = 0;
    bool Halted  = false;

    // only buffer pipeline on STR(|H|B)/STM instructions near PC
    // we still keep PC ahead of course
    s_Pipeline Pipeline;

    Mem* Memory;
    s_scheduler* Scheduler;

    // We want to make this an event that will always be scheduled right away, so that we don't have to think about
    // whether we are in an instruction or not, cause we will never be in this case, and we will always be 4/8 ahead
    // of the next instruction to be executed
    s_event InterruptPoll;
    static SCHEDULER_EVENT(InterruptPollEvent);
    void ScheduleInterruptPoll();

    // bits 27-20 and 7-4 for ARM instructions
    ARMInstructionPtr ARMInstructions[ARMInstructionTableSize] = {};
    // bits 15-6 for THUMB instructions (less are needed, but this allows for more templating)
    THUMBInstructionPtr THUMBInstructions[THUMBInstructionTableSize] = {};

    // LUTs to check if instruction was a branch
    static constexpr auto IsARMBranch = [] {
        std::array<bool, ARMInstructionTableSize> table = {};

        table[ARMHash(0x012fff10)] = true;  // bx
        for (u32 b4567 = 0; b4567 < 0x10; b4567++) {
            for (u32 b20_24 = 0; b20_24 < 0x20; b20_24++) {
                table[ARMHash((0x0f00'0000) | (b20_24 << 20) | (b4567 << 4))] = true;  // swi
                table[ARMHash((0x0a00'0000) | (b20_24 << 20) | (b4567 << 4))] = true;  // b / bl
            }
        }

        return table;
    }();

    static constexpr auto IsTHUMBBranch = [] {
        std::array<bool, THUMBInstructionTableSize> table = {};

        for (u16 h1h2 = 0; h1h2 < 4; h1h2++) {
            table[THUMBHash(0x4700 | (h1h2 << 6))] = true;  // bx
        }

        for (u16 b67 = 0; b67 < 4; b67++) {
            table[THUMBHash(0xbd00 | (b67 << 6))] = true;  // pop PC
            table[THUMBHash(0x4700 | (b67 << 6))] = true;  // swi
        }

        for (u16 b6_10 = 0; b6_10 < 0x40; b6_10++) {
            table[THUMBHash(0xf000 | (b6_10 << 6))] = true;  // long branch with link
            table[THUMBHash(0xd000 | (b6_10 << 6))] = true;  // conditional branch
            table[THUMBHash(0xe000 | (b6_10 << 6))] = true;  // unconditional branch
        }

        return table;
    }();

    template<u32>
    friend constexpr ARMInstructionPtr GetARMInstruction();
    template<u16>
    friend constexpr THUMBInstructionPtr GetTHUMBInstruction();

    void BuildARMTable();
    void BuildTHUMBTable();

    ALWAYS_INLINE void SetNZ(u32 result);
    ALWAYS_INLINE u32 adcs_cv(u32 op1, u32 op2, u32 carry_in);
    ALWAYS_INLINE u32 adds_cv(u32 op1, u32 op2);
    ALWAYS_INLINE u32 sbcs_cv(u32 op1, u32 op2, u32 carry_in);
    ALWAYS_INLINE u32 subs_cv(u32 op1, u32 op2);

    ALWAYS_INLINE void Idle() {
        *timer = Scheduler->PeekEvent();
    }

    [[nodiscard]] ALWAYS_INLINE bool CheckCondition(u8 condition) const;

    // when flushing the pipeline, we _always_ know what state we are in
    // this saves a couple of ASM instructions
    template<bool arm_mode>
    ALWAYS_INLINE void FakePipelineFlush() {
        this->Pipeline.Clear();

        if constexpr(arm_mode) {
            *timer += Memory->GetAccessTime<u32>(static_cast<MemoryRegion>(pc >> 24)) << 1;
            // ARM mode
            this->pc += 4;
        }
        else {
            *timer += Memory->Mem::GetAccessTime<u16>(static_cast<MemoryRegion>(pc >> 24)) << 1;
            // THUMB mode
            this->pc += 2;
        }
    }

    void ChangeMode(Mode NewMode) {
            if (NewMode == static_cast<Mode>(this->CPSR & static_cast<u32>(CPSRFlags::Mode))) {
                return;
            }
            /*
             * FIQ mode is not used in the GBA in general.
             * However, I know that JSMolka's GBA suite (or used to), and ARMWrestler change into FIQ mode,
             * so I keep a bank for that anyway
             *
             * Top mode flag is always 1, so just check the other 4
             * */
            if (unlikely(NewMode == Mode::FIQ)) {
                // bank FIQ registers (registers r8-r12)
                memcpy(FIQBank[0], &Registers[8], 5 * sizeof(u32));
                memcpy( &Registers[8], FIQBank[1], 5 * sizeof(u32));
                // other 2 registers are just banked as normal
            }

            switch (static_cast<Mode>(this->CPSR & static_cast<u32>(CPSRFlags::Mode))) {
                case Mode::FIQ:
                    // coming from FIQ mode, exact same thing in reverse
                    memcpy(FIQBank[1], &Registers[8], 5 * sizeof(u32));
                    memcpy( &Registers[8], FIQBank[0], 5 * sizeof(u32));
                    break;
                case Mode::IRQ:
                    // coming from IRQ, change BIOS readstate
                    Memory->CurrentBIOSReadState = BIOSReadState::AfterIRQ;
                    break;
                case Mode::Supervisor:
                    // coming from SWI, change BIOS readstate
                    Memory->CurrentBIOSReadState = BIOSReadState::AfterSWI;
                    break;
                default:
                    break;
            }

            this->SPLRBank[this->CPSR & 0xf][0] = this->sp;
            this->SPLRBank[this->CPSR & 0xf][1] = this->lr;
            this->SPSRBank[this->CPSR & 0xf]    = this->SPSR;

            this->sp   = this->SPLRBank[static_cast<u8>(NewMode) & 0xf][0];
            this->lr   = this->SPLRBank[static_cast<u8>(NewMode) & 0xf][1];
            this->SPSR = this->SPSRBank[static_cast<u8>(NewMode) & 0xf];

            this->CPSR = (this->CPSR & ~static_cast<u32>(CPSRFlags::Mode)) | static_cast<u8>(NewMode);
            ScheduleInterruptPoll();
        }

    INSTRUCTION(ARMUnimplemented) {
        log_fatal("Unimplemented ARM instruction: %08x at PC = %08x", instruction, this->pc - 8);
    };

    INSTRUCTION(THUMBUnimplemented) {
        log_fatal("Unimplemented THUMB instruction: %04x at PC = %08x", instruction, this->pc - 4);
    }
#define INLINED_INCLUDES
#include "instructions/ARM/Branch.inl"
#include "instructions/ARM/PSRTransfer.inl"
#include "instructions/ARM/DataProcessing.inl"
#include "instructions/ARM/LoadStore.inl"
#include "instructions/ARM/BlockDataTransfer.inl"
#include "instructions/ARM/Multiply.inl"

#include "instructions/THUMB/ALU.inl"
#include "instructions/THUMB/Branch.inl"
#include "instructions/THUMB/LoadStore.inl"

#include "instructions/SWI.inl"
#undef INLINED_INCLUDES
};

u32 ARM7TDMI::adcs_cv(u32 op1, u32 op2, u32 carry_in) {
    // add with carry and set CV
    CPSR &= ~(static_cast<u32>(CPSRFlags::C) | static_cast<u32>(CPSRFlags::V));
    u32 result;
#if defined(FAST_ADD_SUB) && __has_builtin(__builtin_addc)
    u32 carry_out;
    result = __builtin_addc(op1, op2, carry_in, &carry_out);
    if (carry_out) {
        CPSR |= static_cast<u32>(CPSRFlags::C);
    }
#else
    result = op1 + op2 + carry_in;
    CPSR |= (((u64)op1 + (u64)op2 + (u64)carry_in) > 0xffff'ffffULL) ? static_cast<u32>(CPSRFlags::C) : 0;
#endif // addc

    // todo: overflow detection seemed broken for edge cases
#if 0 && defined(FAST_ADD_SUB) && __has_builtin(__builtin_sadd_overflow)
    int _; // we already have the result
    if (__builtin_sadd_overflow(op1 + carry_in, op2, &_) || unlikely(carry_in && (op1 == 0x7fff'ffff))) {
        CPSR |= static_cast<u32>(CPSRFlags::V);
    }
#else
    CPSR |= (((op1 ^ result) & (~op1 ^ op2)) >> 31) ? static_cast<u32>(CPSRFlags::V) : 0;
#endif // add_overflow

    return result;
}

u32 ARM7TDMI::adds_cv(u32 op1, u32 op2) {
    // add and set CV
    return adcs_cv(op1, op2, 0);
}

u32 ARM7TDMI::sbcs_cv(u32 op1, u32 op2, u32 carry_in) {
    // subtract with carry and set CV
    CPSR &= ~(static_cast<u32>(CPSRFlags::C) | static_cast<u32>(CPSRFlags::V));
    u32 result;
#if defined(FAST_ADD_SUB) && __has_builtin(__builtin_subc)
    u32 carry_out;
    // carry is kind of backwards in ARM, so we do !carry_in and !carry_out
    result = __builtin_subc(op1, op2, !carry_in, &carry_out);
    if (!carry_out) {
        CPSR |= static_cast<u32>(CPSRFlags::C);
    }
#else
    result = (u32)(op1 - (op2 - carry_in + 1));
    CPSR |= ((op2 + 1 - carry_in) <= op1) ? static_cast<u32>(CPSRFlags::C) : 0;
#endif // addc

    // todo: overflow detection seemed broken for edge cases
#if 0 && defined(FAST_ADD_SUB) && __has_builtin(__builtin_ssub_overflow)
    int _;  // we already have the result
    if (__builtin_ssub_overflow(op1, op2, &_)) {
        CPSR |= static_cast<u32>(CPSRFlags::V);
    }
#else
    CPSR |= (((op1 ^ op2) & (~op2 ^ result)) >> 31) ? static_cast<u32>(CPSRFlags::V) : 0;
#endif // add_overflow

    return result;
}

u32 ARM7TDMI::subs_cv(u32 op1, u32 op2) {
    // subtract and set CV
    return sbcs_cv(op1, op2, 1);
}

void ARM7TDMI::SetNZ(u32 result) {
    CPSR &= ~(static_cast<u32>(CPSRFlags::N) | static_cast<u32>(CPSRFlags::Z));
    CPSR |= result & 0x8000'0000;
    CPSR |= (result == 0) ? static_cast<u32>(CPSRFlags::Z) : 0;
}

bool ARM7TDMI::CheckCondition(u8 condition) const {
    return ((Conditions[condition] >> (CPSR >> 28)) & 1) != 0;
}

template<bool MakeCache>
bool ARM7TDMI::Step() {
    u32 instruction;

    bool block_end = false;
    if (unlikely(Pipeline.Count)) {
        // we only have stuff in the pipeline if writes near PC happened
        instruction = Pipeline.Dequeue();
        block_end = true;
        if (ARMMode) {
            goto skip_adding_instruction_to_cache_ARM;
        }
        else {
            goto skip_adding_instruction_to_cache_THUMB;
        }
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
        if constexpr(MakeCache) {
            auto instr = CachedInstruction(instruction, ARMInstructions[ARMHash(instruction)]);
            (*CurrentCache)->Instructions.push_back(instr);
            // also check if Rd == pc (does not detect multiplies with pc as destination)
            block_end = IsARMBranch[ARMHash(instruction)]
                     || ((instruction & 0x0000f000) == 0x0000f000)   // any operation with PC as destination
                     || ((instruction & 0x0e108000) == 0x08108000);  // ldm r, { .. pc }
        }
skip_adding_instruction_to_cache_ARM:
        if (CheckCondition(instruction >> 28)) {
            (this->*ARMInstructions[ARMHash(instruction)])(instruction);
        }

        // we handle mode changes in the BX instruction by correcting PC for it there
        // this saves us from doing another check after every instruction
        pc += 4;
    }
    else {
        // THUMB mode
        if constexpr(MakeCache) {
            auto instr = CachedInstruction(instruction, THUMBInstructions[THUMBHash((u16)instruction)]);
            (*CurrentCache)->Instructions.push_back(instr);
            // also check for hi-reg-ops with PC as destination
            block_end = IsTHUMBBranch[THUMBHash((u16)instruction)] || ((instruction & 0xfc87) == 0x4487);
        }
skip_adding_instruction_to_cache_THUMB:
        (this->*THUMBInstructions[THUMBHash((u16)instruction)])(instruction);
        // same here
        pc += 2;
    }

    if constexpr (MakeCache) {
        if (block_end || !(corrected_pc & (Mem::InstructionCacheBlockSizeBytes - 1))) {
            // branch or block alignment
            return true;
        }
        return false;
    }
    else {
        // return whether we might have a cache block
        return InCacheRegion(corrected_pc);
    }
}
