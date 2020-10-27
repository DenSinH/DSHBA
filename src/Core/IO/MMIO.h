#pragma once

#include "../Mem/MemoryHelpers.h"
#include "../PPU/PPU.h"

#include "../Scheduler/scheduler.h"

#include "default.h"
#include "helpers.h"

enum class KeypadButton : u16 {
    A      = 0x0001,
    B      = 0x0002,
    Select = 0x0004,
    Start  = 0x0008,
    Right  = 0x0010,
    Left   = 0x0020,
    Up     = 0x0040,
    Down   = 0x0080,
    R      = 0x0100,
    L      = 0x0200,
};

class MMIO;
class ARM7TDMI;

typedef void (MMIO::*IOWriteCallback)(u16 value);
typedef u16 (MMIO::*IOReadPrecall)();

#define READ_PRECALL(_name) u16 _name()
#define WRITE_CALLBACK(_name) void _name(u16 value)

class MMIO {

    public:

        MMIO(GBAPPU* ppu, ARM7TDMI* cpu, s_scheduler* scheduler);
        ~MMIO() {};

        // expect masked address:
        template<typename T>
        inline T Read(u32 address);
        template<typename T>
        inline void Write(u32 address, T value);

    private:
        friend SCHEDULER_EVENT(GBAPPU::BufferScanlineEvent); // allow registers to be buffered
        friend void ParseInput(struct s_controller* controller);   // joypad input
        friend class Initializer;

        static SCHEDULER_EVENT(HBlankFlagEvent);
        s_event HBlankFlag = {};
        static SCHEDULER_EVENT(VBlankFlagEvent);
        s_event VBlankFlag = {};

        READ_PRECALL(ReadKEYINPUT);
        READ_PRECALL(ReadDISPSTAT);
        WRITE_CALLBACK(WriteDISPSTAT);

        u16 DISPSTAT = 0;
        u16 KEYINPUT = 0xffff;  // flipped

        ARM7TDMI* CPU;
        GBAPPU* PPU;
        s_scheduler* Scheduler;

        u8 Registers[0x400] = {};

        /*
         * In general, registers wont have a callback
         * Mostly registers like IE (causing interrupt polling), and joypad registers need it
         *
         * Readonly registers, we should only define a ReadPrecall for, not having a WriteCallback will just mean
         * it won't do anything when written to then.
         * */
        IOWriteCallback WriteCallback[0x400 >> 1] = {};
        IOReadPrecall ReadPrecall[0x400 >> 1]     = {};
};

template<>
inline u32 MMIO::Read<u32>(u32 address) {
    // 32 bit writes are a little different
    if (unlikely(ReadPrecall[address >> 1])) {
        WriteArray<u16>(Registers, address, (this->*ReadPrecall[address >> 1])());
    }
    if (unlikely(ReadPrecall[1 + (address >> 1)])) {
        WriteArray<u16>(Registers, address + 2, (this->*ReadPrecall[1 + (address >> 1)])());
    }

    return ReadArray<u32>(Registers, address);
}

template<typename T>
T MMIO::Read(u32 address) {
    if (unlikely(ReadPrecall[address >> 1])) {
        WriteArray<u16>(Registers, address, (this->*ReadPrecall[address >> 1])());
    }
    return ReadArray<T>(Registers, address);
}

template<>
inline void MMIO::Write<u32>(u32 address, u32 value) {
    // 32 bit writes are a little different
    WriteArray<u32>(Registers, address, value);

    if (unlikely(WriteCallback[address >> 1])) {
        // little endian:
        (this->*WriteCallback[address >> 1])(value);
    }
    if (unlikely(WriteCallback[1 + (address >> 1)])) {
        (this->*WriteCallback[1 + (address >> 1)])(value >> 16);
    }
}

template<typename T>
void MMIO::Write(u32 address, T value) {
    WriteArray<T>(Registers, address, value);

    if (unlikely(ReadPrecall[address >> 1])) {
        WriteArray<u16>(Registers, address, (this->*ReadPrecall[address >> 1])());
    }
}