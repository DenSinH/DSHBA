#pragma once

#include "Pipeline.h"

#include "../Mem/Mem.h"

#include "default.h"
#include "helpers.h"
#include "flags.h"

class ARM7TDMI;

#define sp Registers[13]
#define lr Registers[14]
#define pc Registers[15]

const size_t ARMInstructionTableSize   = 0x1000;
const size_t THUMBInstructionTableSize = 0x400;

typedef union s_CPSR {
    struct {
        u8 Mode: 5;
        unsigned T: 1;
        unsigned F: 1;
        unsigned I: 1;
        unsigned: 20;  // reserved
        unsigned V: 1;
        unsigned C: 1;
        unsigned Z: 1;
        unsigned N: 1;
    };

    u32 raw;
} s_CPSR;

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

    explicit ARM7TDMI(Mem* memory) {
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
        CPSR = SetCPSR(0x6000'001f);

        this->FlushPipeline();
        this->pc += 4;
    }
    void Step();

private:
    friend class Initializer;
    friend class GBA;

    // this is to hack CLion into thinking we can access everything
    friend class ARM7TDMI_INL;

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

    s_CPSR CPSR         = {};
    s_CPSR SPSR         = {};
    s_CPSR SPSRBank[16] = {};
    u32 SPLRBank[16][2] = {};
    u32 Registers[16]   = {};

    s_Pipeline Pipeline;

    Mem* Memory;

    // bits 27-20 and 7-4 for ARM instructions
    ARMInstructionPtr ARMInstructions[ARMInstructionTableSize];
    // bits 15-6 for THUMB instructions (less are needed, but this allows for more templating)
    THUMBInstructionPtr THUMBInstructions[THUMBInstructionTableSize];

    static inline __attribute__((always_inline)) constexpr u32 ARMHash(u32 instruction) {
        return ((instruction & 0x0ff0'0000) >> 16) | ((instruction & 0x00f0) >> 4);
    }

    static inline __attribute__((always_inline)) constexpr u32 THUMBHash(u16 instruction) {
        return ((instruction & 0xffc0) >> 6);
    }

    static inline s_CPSR SetCPSR(u32 value) {
#ifdef SAFE_CPSR_CASTING
        return (s_CPSR) {
                .N = (value & static_cast<u32>(CPSRFlags::N)) != 0,
                .Z = (value & static_cast<u32>(CPSRFlags::Z)) != 0,
                .C = (value & static_cast<u32>(CPSRFlags::C)) != 0,
                .V = (value & static_cast<u32>(CPSRFlags::V)) != 0,
                .I = (value & static_cast<u32>(CPSRFlags::I)) != 0,
                .F = (value & static_cast<u32>(CPSRFlags::F)) != 0,
                .T = (value & static_cast<u32>(CPSRFlags::T)) != 0,
                .Mode = static_cast<u8>((value & static_cast<u32>(CPSRFlags::Mode))),
        };
#else
        return (s_CPSR) { .raw = value };
#endif
    }

    static inline u32 GetCPSR(s_CPSR CPSR) {
#ifdef SAFE_CPSR_CASTING
        return
                ((u32)CPSR.N << 31) |
                ((u32)CPSR.Z << 30) |
                ((u32)CPSR.C << 29) |
                ((u32)CPSR.V << 28) |
                ((u32)CPSR.I << 7)  |
                ((u32)CPSR.F << 6)  |
                ((u32)CPSR.T << 5)  |
                static_cast<u32>(static_cast<u8>(CPSR.Mode));
#else
        return CPSR.raw;
#endif
    }

    template<u32>
    friend constexpr ARMInstructionPtr GetARMInstruction();
    template<u16>
    friend constexpr THUMBInstructionPtr GetTHUMBInstruction();

    void BuildARMTable();
    void BuildTHUMBTable();

    inline __attribute__((always_inline)) void SetCVAdd(u32 op1, u32 op2, u32 result);
    inline __attribute__((always_inline)) void SetCVAddC(u32 op1, u32 op2, u32 c, u32 result);
    inline __attribute__((always_inline)) void SetCVSub(u32 op1, u32 op2, u32 result);
    inline __attribute__((always_inline)) void SetCVSubC(u32 op1, u32 op2, u32 c, u32 result);
    inline __attribute__((always_inline)) void SetNZ(u32 result);
    [[nodiscard]] inline bool CheckCondition(u8 condition) const;
    void FlushPipeline();

    void ChangeMode(Mode NewMode) {
            if (NewMode == static_cast<Mode>(this->CPSR.Mode)) {
                return;
            }
            /*
             * FIQ mode is not used in the GBA in general.
             * However, I know that JSMolka's GBA suite (used to) change into FIQ mode, so I keep a bank for that anyway
             * */
            this->SPLRBank[this->CPSR.Mode & 0xf][0] = this->sp;
            this->SPLRBank[this->CPSR.Mode & 0xf][1] = this->lr;
            this->SPSRBank[this->CPSR.Mode & 0xf]    = this->SPSR;

            this->sp   = this->SPLRBank[static_cast<u8>(NewMode) & 0xf][0];
            this->lr   = this->SPLRBank[static_cast<u8>(NewMode) & 0xf][1];
            this->SPSR = this->SPSRBank[static_cast<u8>(NewMode) & 0xf];

            this->CPSR.Mode = static_cast<u8>(NewMode);
        }

    void ARMUnimplemented(u32 instruction) {
        log_fatal("Unimplemented ARM instruction: %08x at PC = %08x", instruction, this->pc - 8);
    };

    void THUMBUnimplemented(u16 instruction) {
        log_fatal("Unimplemented THUMB instruction: %04x at PC = %08x", instruction, this->pc - 4);
    }
#define INLINED_INCLUDES
#include "instructions/ARM/Branch.inl"
#include "instructions/ARM/DataProcessing.inl"
#include "instructions/ARM/LoadStore.inl"
#include "instructions/ARM/BlockDataTransfer.inl"
#include "instructions/ARM/Multiply.inl"

#include "instructions/THUMB/ALU.inl"
#include "instructions/THUMB/Branch.inl"
#include "instructions/THUMB/LoadStore.inl"
#undef INLINED_INCLUDES
};

void ARM7TDMI::SetCVAdd(u32 op1, u32 op2, u32 result)
{
    CPSR.C = (op1 > ~op2 ? 1 : 0);
    CPSR.V = (((op1 ^ result) & (~op1 ^ op2)) >> 31);
}

void ARM7TDMI::SetCVAddC(u32 op1, u32 op2, u32 c, u32 result)
{
    // for ADC
    CPSR.C = ((op1 + op2 == 0xffff'ffff) && c) || (op1 + c > ~op2 ? 1 : 0);
    CPSR.V = (((op1 ^ result) & (~op1 ^ op2)) >> 31);
}

void ARM7TDMI::SetCVSub(u32 op1, u32 op2, u32 result)
{
    // for op1 - op2
    CPSR.C = (op2 <= op1 ? 1 : 0);
    CPSR.V = (((op1 ^ op2) & (~op2 ^ result)) >> 31);
}

void ARM7TDMI::SetCVSubC(u32 op1, u32 op2, u32 c, u32 result)
{
    // for op1 - op2
    CPSR.C = (op2 + 1 - c <= op1 ? 1 : 0);
    CPSR.V = (((op1 ^ op2) & (~op2 ^ result)) >> 31);
}

void ARM7TDMI::SetNZ(u32 result) {
    CPSR.N = (i32)result < 0;
    CPSR.Z = result == 0;
}