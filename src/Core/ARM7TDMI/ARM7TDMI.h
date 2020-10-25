#pragma once

#include "Pipeline.h"

#include "../Mem/Mem.h"
#include "../Scheduler/scheduler.h"

#include "default.h"
#include "helpers.h"
#include "flags.h"

class ARM7TDMI;

#define sp Registers[13]
#define lr Registers[14]
#define pc Registers[15]

const size_t ARMInstructionTableSize   = 0x1000;
const size_t THUMBInstructionTableSize = 0x400;

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

typedef void (ARM7TDMI::*ARMInstructionPtr)(u32 instruction);
typedef void (ARM7TDMI::*THUMBInstructionPtr)(u16 instruction);

class ARM7TDMI {
public:
    u64 timer = 0;

    explicit ARM7TDMI(s_scheduler* scheduler, Mem* memory) {
        this->Scheduler = scheduler;
        this->Memory = memory;

        this->BuildARMTable();
        this->BuildTHUMBTable();
    }
    ~ARM7TDMI() {

    };

    void SkipBIOS() {
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
    void Step();
    void PipelineReflush();

private:
    friend class Initializer;
    friend class GBA;

    // this is to hack CLion into thinking we can access everything
    friend class ARM7TDMI_INL;

    friend void benchmark();

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

    u32 CPSR         = {};
    u32 SPSR         = {};
    u32 SPSRBank[16] = {};
    u32 SPLRBank[16][2] = {};
    u32 Registers[16]   = {};

    // todo: only buffer pipeline on STR(|H|B) instructions near PC
    // we still keep PC ahead of course
    s_Pipeline Pipeline;

    Mem* Memory;
    s_scheduler* Scheduler;

    // bits 27-20 and 7-4 for ARM instructions
    ARMInstructionPtr ARMInstructions[ARMInstructionTableSize] = {};
    // bits 15-6 for THUMB instructions (less are needed, but this allows for more templating)
    THUMBInstructionPtr THUMBInstructions[THUMBInstructionTableSize] = {};

    static ALWAYS_INLINE constexpr u32 ARMHash(u32 instruction) {
        return ((instruction & 0x0ff0'0000) >> 16) | ((instruction & 0x00f0) >> 4);
    }

    static ALWAYS_INLINE constexpr u32 THUMBHash(u16 instruction) {
        return ((instruction & 0xffc0) >> 6);
    }

    template<u32>
    friend constexpr ARMInstructionPtr GetARMInstruction();
    template<u16>
    friend constexpr THUMBInstructionPtr GetTHUMBInstruction();

    void BuildARMTable();
    void BuildTHUMBTable();

//    ALWAYS_INLINE void SetCVAdd(u32 op1, u32 op2, u32 result);
//    ALWAYS_INLINE void SetCVAddC(u64 op1, u64 op2, u32 c, u32 result);
//    ALWAYS_INLINE void SetCVSub(u32 op1, u32 op2, u32 result);
//    ALWAYS_INLINE void SetCVSubC(u64 op1, u64 op2, u32 c, u32 result);
    ALWAYS_INLINE void SetNZ(u32 result);

    ALWAYS_INLINE u32 adcs_cv(u32 op1, u32 op2, u32 carry_in);
    ALWAYS_INLINE u32 adds_cv(u32 op1, u32 op2);
    ALWAYS_INLINE u32 sbcs_cv(u32 op1, u32 op2, u32 carry_in);
    ALWAYS_INLINE u32 sbcs_cv_old(u32 op1, u32 op2, u32 carry_in);
    ALWAYS_INLINE u32 subs_cv(u32 op1, u32 op2);

    ALWAYS_INLINE void Idle() {
        timer = peek_event(Scheduler);
    }

    [[nodiscard]] inline bool CheckCondition(u8 condition) const;
    void FakePipelineFlush();

    void ChangeMode(Mode NewMode) {
            if (NewMode == static_cast<Mode>(this->CPSR & static_cast<u32>(CPSRFlags::Mode))) {
                return;
            }
            /*
             * FIQ mode is not used in the GBA in general.
             * However, I know that JSMolka's GBA suite (used to) change into FIQ mode, so I keep a bank for that anyway
             *
             * Top mode flag is always 1, so just check the other 4
             * */
            this->SPLRBank[this->CPSR & 0xf][0] = this->sp;
            this->SPLRBank[this->CPSR & 0xf][1] = this->lr;
            this->SPSRBank[this->CPSR & 0xf]    = this->SPSR;

            this->sp   = this->SPLRBank[static_cast<u8>(NewMode) & 0xf][0];
            this->lr   = this->SPLRBank[static_cast<u8>(NewMode) & 0xf][1];
            this->SPSR = this->SPSRBank[static_cast<u8>(NewMode) & 0xf];

            this->CPSR = (this->CPSR & ~static_cast<u32>(CPSRFlags::Mode)) | static_cast<u8>(NewMode);
        }

    void ARMUnimplemented(u32 instruction) {
        log_fatal("Unimplemented ARM instruction: %08x at PC = %08x", instruction, this->pc - 8);
    };

    void THUMBUnimplemented(u16 instruction) {
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
    CPSR |= ((op1 + op2 + carry_in) > 0xffff'ffffULL) ? static_cast<u32>(CPSRFlags::C) : 0;
#endif // addc

#if defined(FAST_ADD_SUB) && __has_builtin(__builtin_sadd_overflow)
    int _; // we already have the result
    if (__builtin_sadd_overflow(op1 + carry_in, op2, &_)) {
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

#if defined(FAST_ADD_SUB) && __has_builtin(__builtin_ssub_overflow)
    int _;  // we already have the result
    if (__builtin_ssub_overflow(op1, op2 + 1 - carry_in, &_)) {
        CPSR |= static_cast<u32>(CPSRFlags::V);
    }
#else
    CPSR |= (((op1 ^ op2) & (~op2 ^ result)) >> 31) ? static_cast<u32>(CPSRFlags::V) : 0;
#endif // add_overflow

    return result;
}

u32 ARM7TDMI::sbcs_cv_old(u32 op1, u32 op2, u32 carry_in) {
    // subtract with carry and set CV
    CPSR &= ~(static_cast<u32>(CPSRFlags::C) | static_cast<u32>(CPSRFlags::V));
    u32 result;
    result = (u32)(op1 - (op2 - carry_in + 1));
    CPSR |= ((op2 + 1 - carry_in) <= op1) ? static_cast<u32>(CPSRFlags::C) : 0;
    CPSR |= (((op1 ^ op2) & (~op2 ^ result)) >> 31) ? static_cast<u32>(CPSRFlags::V) : 0;

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