#pragma once

#include "Registers.h"
#include "IOFlags.h"
#include "Interrupts.h"

#include "../Mem/MemoryHelpers.h"
#include "../PPU/PPU.h"

#include "../Scheduler/scheduler.h"

#include "default.h"
#include "helpers.h"

#include <array>

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
class Mem;
class ARM7TDMI;

typedef void (MMIO::*IOWriteCallback)(u16 value);
typedef u16 (MMIO::*IOReadPrecall)();

typedef struct s_DMAData {
    u32 SAD;
    u32 DAD;
    u16 CNT_L;
    u16 CNT_H;
    u32 CNT_L_MAX;
} s_DMAData;

struct s_TimerData {
    struct {
        u16 CNT_L;  // Reload value
        u16 CNT_H;
    } Register;

    u8 PrescalerShift;   // shift amount
    u32 Counter;         // used in count-up mode
    u64 TriggerTime;     // used in direct mode

    s_event Overflow;

    void FlushDirect(u64 CurrentTime) {
        // we assume that Reload + (TriggerTime - CurrentTime) / Prescaler < 0x10000 at all times!
        u64 dt = (CurrentTime - TriggerTime) >> PrescalerShift;
        Counter += dt;
        TriggerTime += dt << PrescalerShift;  // don't lose resolution
    }
};

#define READ_PRECALL(_name) u16 _name()
#define WRITE_CALLBACK(_name) void _name(u16 value)

class MMIO {

public:

    MMIO(GBAPPU* ppu, ARM7TDMI* cpu, Mem* memory, s_scheduler* scheduler);
    ~MMIO() {};

    // expect masked address:
    template<typename T> inline T Read(u32 address);
    template<typename T> inline void Write(u32 address, T value);

private:
    friend void GBAPPU::BufferScanline(u32); // allow registers to be buffered
    friend void ParseInput(struct s_controller* controller);   // joypad input
    friend class Mem;
    friend class Initializer;

    void TriggerInterrupt(u16 interrupt);

    /*============== LCD ==============*/
    u16 DISPSTAT = 0;
    u16 VCount   = 0;

    static SCHEDULER_EVENT(HBlankEvent);
    s_event HBlank = {};
    static SCHEDULER_EVENT(VBlankEvent);
    s_event VBlank = {};
    static SCHEDULER_EVENT(HBlankFlagEvent);
    s_event HBlankFlag = {};

    u32 ReferenceLine2 = 0;
    u32 ReferenceLine3 = 0;
    template<bool BG2> WRITE_CALLBACK(WriteReferencePoint);
    READ_PRECALL(ReadDISPSTAT);
    WRITE_CALLBACK(WriteDISPSTAT);
    READ_PRECALL(ReadVCount);
    READ_PRECALL(ReadKEYINPUT);

    /*============== DMA ==============*/
    bool DMAEnabled[4]       = {};
    s_DMAData DMAData[4]     = {};  // shadow registers on DMA enable
    void TriggerDMAChannel(u32 x);
    void TriggerDMATiming(DMACNT_HFlags start_timing);
    template<u8 x> WRITE_CALLBACK(WriteDMAxCNT_H);
    template<u8 x> READ_PRECALL(ReadDMAxCNT_H);

    /*============= Timers =============*/
    s_TimerData Timer[4]     = {};
    template<u8 x> static SCHEDULER_EVENT(TimerOverflowEvent);
    template<u8 x> void OverflowTimer();  // return next timestamp of overflow
    template<u8 x> WRITE_CALLBACK(WriteTMxCNT_L);
    template<u8 x> WRITE_CALLBACK(WriteTMxCNT_H);
    template<u8 x> READ_PRECALL(ReadTMxCNT_L);

    /*=============== COM ===============*/
    WRITE_CALLBACK(WriteSIOCNT);  // mostly used to just generate an IRQ whenever necessary

    /*============ Interrupts ===========*/
    // IE/IME we won't read back, data can't be changed externally
    WRITE_CALLBACK(WriteIME);
    WRITE_CALLBACK(WriteIE);
    WRITE_CALLBACK(WriteIF);  // IF has special writes
    READ_PRECALL(ReadIF);     // and can be changed externally
    WRITE_CALLBACK(WritePOSTFLG_HALTCNT);

    u16 KEYINPUT = 0xffff;  // flipped

    ARM7TDMI* CPU;
    GBAPPU* PPU;
    Mem* Memory;
    s_scheduler* Scheduler;

    u8 Registers[0x400]      = {};

    /*
     * In general, registers wont have a callback
     * Mostly registers like IE (causing interrupt polling), and joypad registers need it
     *
     * Readonly registers, we should only define a ReadPrecall for, not having a WriteCallback will just mean
     * it won't do anything when written to then.
     * */
    static constexpr auto WriteCallback = [] {
        std::array<IOWriteCallback, (0x400 >> 1)> table = {nullptr};
        table[static_cast<u32>(IORegister::DISPSTAT) >> 1]  = &MMIO::WriteDISPSTAT;

        table[static_cast<u32>(IORegister::BG2X) >> 1] = &MMIO::WriteReferencePoint<true>;
        table[(static_cast<u32>(IORegister::BG2X) >> 1) + 1] = &MMIO::WriteReferencePoint<true>;  // upper part
        table[static_cast<u32>(IORegister::BG2Y) >> 1] = &MMIO::WriteReferencePoint<true>;
        table[(static_cast<u32>(IORegister::BG2Y) >> 1) + 1] = &MMIO::WriteReferencePoint<true>;  // upper part

        table[static_cast<u32>(IORegister::BG3X) >> 1] = &MMIO::WriteReferencePoint<false>;
        table[(static_cast<u32>(IORegister::BG3X) >> 1) + 1] = &MMIO::WriteReferencePoint<false>;  // upper part
        table[static_cast<u32>(IORegister::BG3Y) >> 1] = &MMIO::WriteReferencePoint<false>;
        table[(static_cast<u32>(IORegister::BG3Y) >> 1) + 1] = &MMIO::WriteReferencePoint<false>;  // upper part

        table[static_cast<u32>(IORegister::TM0CNT_L) >> 1] = &MMIO::WriteTMxCNT_L<0>;
        table[static_cast<u32>(IORegister::TM1CNT_L) >> 1] = &MMIO::WriteTMxCNT_L<1>;
        table[static_cast<u32>(IORegister::TM2CNT_L) >> 1] = &MMIO::WriteTMxCNT_L<2>;
        table[static_cast<u32>(IORegister::TM3CNT_L) >> 1] = &MMIO::WriteTMxCNT_L<3>;

        table[static_cast<u32>(IORegister::TM0CNT_H) >> 1] = &MMIO::WriteTMxCNT_H<0>;
        table[static_cast<u32>(IORegister::TM1CNT_H) >> 1] = &MMIO::WriteTMxCNT_H<1>;
        table[static_cast<u32>(IORegister::TM2CNT_H) >> 1] = &MMIO::WriteTMxCNT_H<2>;
        table[static_cast<u32>(IORegister::TM3CNT_H) >> 1] = &MMIO::WriteTMxCNT_H<3>;

        table[static_cast<u32>(IORegister::DMA0CNT_H) >> 1] = &MMIO::WriteDMAxCNT_H<0>;
        table[static_cast<u32>(IORegister::DMA1CNT_H) >> 1] = &MMIO::WriteDMAxCNT_H<1>;
        table[static_cast<u32>(IORegister::DMA2CNT_H) >> 1] = &MMIO::WriteDMAxCNT_H<2>;
        table[static_cast<u32>(IORegister::DMA3CNT_H) >> 1] = &MMIO::WriteDMAxCNT_H<3>;

#ifdef STUB_SIO
        table[static_cast<u32>(IORegister::SIOCNT) >> 1] = &MMIO::WriteSIOCNT;
#endif

        table[static_cast<u32>(IORegister::IME)      >> 1]  = &MMIO::WriteIME;
        table[static_cast<u32>(IORegister::IE)       >> 1]  = &MMIO::WriteIE;
        table[static_cast<u32>(IORegister::IF)       >> 1]  = &MMIO::WriteIF;
        table[static_cast<u32>(IORegister::HALTCNT)  >> 1]  = &MMIO::WritePOSTFLG_HALTCNT;
        return table;
    }();

    static constexpr auto ReadPrecall = [] {
        std::array<IOReadPrecall, (0x400 >> 1)> table = {nullptr};
        table[static_cast<u32>(IORegister::DISPSTAT) >> 1] = &MMIO::ReadDISPSTAT;
        table[static_cast<u32>(IORegister::VCOUNT)   >> 1] = &MMIO::ReadVCount;

        table[static_cast<u32>(IORegister::DMA0CNT_H) >> 1] = &MMIO::ReadDMAxCNT_H<0>;
        table[static_cast<u32>(IORegister::DMA1CNT_H) >> 1] = &MMIO::ReadDMAxCNT_H<1>;
        table[static_cast<u32>(IORegister::DMA2CNT_H) >> 1] = &MMIO::ReadDMAxCNT_H<2>;
        table[static_cast<u32>(IORegister::DMA3CNT_H) >> 1] = &MMIO::ReadDMAxCNT_H<3>;

        table[static_cast<u32>(IORegister::TM0CNT_L) >> 1] = &MMIO::ReadTMxCNT_L<0>;
        table[static_cast<u32>(IORegister::TM1CNT_L) >> 1] = &MMIO::ReadTMxCNT_L<1>;
        table[static_cast<u32>(IORegister::TM2CNT_L) >> 1] = &MMIO::ReadTMxCNT_L<2>;
        table[static_cast<u32>(IORegister::TM3CNT_L) >> 1] = &MMIO::ReadTMxCNT_L<3>;

        table[static_cast<u32>(IORegister::KEYINPUT) >> 1] = &MMIO::ReadKEYINPUT;

        table[static_cast<u32>(IORegister::IF)       >> 1] = &MMIO::ReadIF;

        return table;
    }();

    // registers that do not have a callback might have a write mask
    // if there is a write callback, I will assume that I also masked stuff there anyway
    static constexpr auto WriteMask = [] {
        std::array<u16, (0x400 >> 1)> table = {};

        // fill with 1s
        for (int i = 0; i < table.size(); i++) {
            table[i] = 0xffff;
        }

        table[(static_cast<u32>(IORegister::BG0CNT) >> 1)] = 0xdfff;
        table[(static_cast<u32>(IORegister::BG1CNT) >> 1)] = 0xdfff;

        table[(static_cast<u32>(IORegister::BG2X) >> 1) + 1] = 0x0fff;    // upper part
        table[(static_cast<u32>(IORegister::BG2Y) >> 1) + 1] = 0x0fff;    // upper part
        table[(static_cast<u32>(IORegister::BG3X) >> 1) + 1] = 0x0fff;    // upper part
        table[(static_cast<u32>(IORegister::BG3Y) >> 1) + 1] = 0x0fff;    // upper part

        table[(static_cast<u32>(IORegister::TM0CNT_H) >> 1)] = 0x00c3;
        table[(static_cast<u32>(IORegister::TM1CNT_H) >> 1)] = 0x00c7;
        table[(static_cast<u32>(IORegister::TM2CNT_H) >> 1)] = 0x00c7;
        table[(static_cast<u32>(IORegister::TM3CNT_H) >> 1)] = 0x00c7;

        table[(static_cast<u32>(IORegister::DMA0SAD) >> 1) + 1] = 0x07ff;  // upper part
        table[(static_cast<u32>(IORegister::DMA1SAD) >> 1) + 1] = 0x0fff;  // upper part
        table[(static_cast<u32>(IORegister::DMA2SAD) >> 1) + 1] = 0x0fff;  // upper part
        table[(static_cast<u32>(IORegister::DMA3SAD) >> 1) + 1] = 0x0fff;  // upper part

        table[(static_cast<u32>(IORegister::DMA0DAD) >> 1) + 1] = 0x07ff;  // upper part
        table[(static_cast<u32>(IORegister::DMA1DAD) >> 1) + 1] = 0x07ff;  // upper part
        table[(static_cast<u32>(IORegister::DMA2DAD) >> 1) + 1] = 0x07ff;  // upper part
        table[(static_cast<u32>(IORegister::DMA3DAD) >> 1) + 1] = 0x0fff;  // upper part

        table[(static_cast<u32>(IORegister::DMA0CNT_L) >> 1)] = 0x3fff;
        table[(static_cast<u32>(IORegister::DMA1CNT_L) >> 1)] = 0x3fff;
        table[(static_cast<u32>(IORegister::DMA2CNT_L) >> 1)] = 0x3fff;

        return table;
    }();
};

template<bool BG2>
WRITE_CALLBACK(MMIO::WriteReferencePoint) {
    // through little endianness, we can write this here
    if (VCount < VISIBLE_SCREEN_HEIGHT) {
        if constexpr(BG2) {
            ReferenceLine2 = VCount;
        }
        else {
            ReferenceLine3 = VCount;
        }
    }
}

#include "IOReadWrite.inl"  // Read/Write templated functions
#include "IODMA.inl"        // DMA related inlined functions
#include "Timers.inl"
