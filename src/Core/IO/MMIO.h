#pragma once

#include "../Mem/MemoryHelpers.h"
#include "../PPU/PPU.h"

#include "../Scheduler/scheduler.h"

#include "default.h"
#include "helpers.h"

class MMIO;
class ARM7TDMI;

typedef void (MMIO::*IOWriteCallback)(u16 value);
typedef u16 (MMIO::*IOReadPrecall)();

class MMIO {

    public:

        MMIO(GBAPPU* ppu, ARM7TDMI* cpu) {
            PPU = ppu;
            cpu = cpu;
        }

        // expect masked address:
        template<typename T>
        inline T Read(u32 address);
        template<typename T>
        inline void Write(u32 address, T value);

    private:
        friend SCHEDULER_EVENT(GBAPPU::BufferScanlineEvent); // allow registers to be buffered

        ARM7TDMI* CPU;
        GBAPPU* PPU;

        u8 Registers[0x400] = {};
        IOWriteCallback WriteCallback[0x400 >> 1] = {};
        IOReadPrecall ReadPrecall[0x400 >> 1]    = {};
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