#pragma once

#include "default.h"
#include "helpers.h"
#include "flags.h"
#include "MemoryHelpers.h"

#include <math.h>

#include <string>
#include <functional>
#include <utility>

enum class MemoryRegion {
    BIOS   = 0x00,
    Unused = 0x01,
    eWRAM  = 0x02,
    iWRAM  = 0x03,
    IO     = 0x04,
    PAL    = 0x05,
    VRAM   = 0x06,
    OAM    = 0x07,
    ROM_L  = 0x08,
    ROM_H  = 0x09,
    ROM_L1 = 0x0a,
    ROM_H1 = 0x0b,
    ROM_L2 = 0x0c,
    ROM_H2 = 0x0d,
    SRAM   = 0x0e
};

/*
 * Every time we draw a scanline, we buffer the video memory data to a separate array, to be read in another thread
 * */
typedef struct s_UpdateRange {
    u32 min;
    u32 max;
} s_UpdateRange;

typedef u8 PALMEM[0x400];
typedef u8 OAMMEM[0x400];
typedef u8 VRAMMEM[0x1'8000];
typedef u8 LCDIO[0x54];

class Mem {
public:
    Mem(u32* pc_ptr, std::function<void(void)> reflush);
    ~Mem();

    template<typename T, bool count> T Read(u32 address);
    template<typename T, bool count, bool do_reflush> void Write(u32 address, T value);
    template<typename T>
    ALWAYS_INLINE void WriteVRAM(u32 address, u32 mask, T value);

    void LoadROM(const std::string& file_path);
    void LoadBIOS(const std::string& file_path);

private:
    friend class Initializer;
    friend class GBAPPU;

    u32* pc_ptr;
    std::function<void(void)> Reflush;

    u8 BIOS  [0x4000]        = {};
    u8 eWRAM [0x4'0000]      = {};
    u8 iWRAM [0x8000]        = {};
    u8 IO    [0x400]         = {};
    PALMEM PAL               = {};
    VRAMMEM VRAM             = {};
    s_UpdateRange VRAMUpdate = { .min=sizeof(VRAMMEM), .max=0 };
    OAMMEM OAM               = {};
    u8 ROM   [0x0200'0000]   = {};
};

static u32 stubber = 0;

template<typename T, bool count>
T Mem::Read(u32 address) {
    switch (static_cast<MemoryRegion>(address >> 24)) {
        case MemoryRegion::BIOS:
            return ReadArray<T>(BIOS, address & 0x3fff);
        case MemoryRegion::Unused:
            return 0;
        case MemoryRegion::eWRAM:
            return ReadArray<T>(eWRAM, address & 0x3'ffff);
        case MemoryRegion::iWRAM:
            return ReadArray<T>(iWRAM, address & 0x7fff);
        case MemoryRegion::IO:
            if ((address & 0x00ff'ffff) > 0x3ff) {
                return 0;
            }
            // todo: read pre-calls
            if (address == 0x0400'0004) {
                // stub DISPSTAT
                return (T)(stubber ^= 0xffff'ffff);
            }

            return (T)0xffff'ffff;// ReadArray<T>(IO, address & 0x3ff);
        case MemoryRegion::PAL:
            return ReadArray<T>(PAL, address & 0x3ff);
        case MemoryRegion::VRAM:
            if ((address & 0x1'ffff) < 0x1'0000) {
                return ReadArray<T>(VRAM, address & 0xffff);
            }
            return ReadArray<T>(VRAM, 0x1'0000 | (address & 0x7fff));
        case MemoryRegion::OAM:
            return ReadArray<T>(OAM, address & 0x3ff);
        case MemoryRegion::ROM_L1:
        case MemoryRegion::ROM_L2:
            // todo: EEPROM attempts
        case MemoryRegion::ROM_L:
        case MemoryRegion::ROM_H1:
        case MemoryRegion::ROM_H2:
        case MemoryRegion::ROM_H:
            // todo: EEPROM attempts
            return ReadArray<T>(ROM, address & 0x01ff'ffff);
        case MemoryRegion::SRAM:
            log_warn("SRAM read @0x%08x", address);
            return 0;
        default:
            return 0;
    }
}

template<typename T>
ALWAYS_INLINE void Mem::WriteVRAM(u32 address, u32 mask, T value) {
    if (ReadArray<T>(VRAM, address & mask) != value) {
        if ((address & mask) > VRAMUpdate.max) {
            VRAMUpdate.max = (address & mask);
        }
        if (((address + sizeof(T)) & mask) < VRAMUpdate.min) {
            VRAMUpdate.min = ((address + sizeof(T)) & mask);
        }
    }
}

template<typename T, bool count, bool do_reflush>
void Mem::Write(u32 address, T value) {
    // We only want to re-flush the ARM7TDMI pipeline if we are in an instruction when it happens
    // DMAs, we assume that PC is not in the DMA-ed part of the code
#define NEAR_PC(mask) ((address >> 24) == (*pc_ptr >> 24)) && ((std::abs(int(address - *pc_ptr)) & (mask)) <= 8)

    switch (static_cast<MemoryRegion>(address >> 24)) {
        case MemoryRegion::BIOS:
            // BIOS is read-only
        case MemoryRegion::Unused:
            return;
        case MemoryRegion::eWRAM:
            if constexpr(do_reflush) {
                if (unlikely(NEAR_PC(0x3'ffff))) {
                    Reflush();
                }
            }
            WriteArray<T>(eWRAM, address & 0x3'ffff, value);
            return;
        case MemoryRegion::iWRAM:
            if constexpr(do_reflush) {
                if (unlikely(NEAR_PC(0x7fff))) {
                    Reflush();
                }
            }
            WriteArray<T>(iWRAM, address & 0x7fff, value);
            return;
        case MemoryRegion::IO:
#ifdef CHECK_INVALID_REFLUSHES
            if constexpr(do_reflush) {
                if (unlikely(NEAR_PC(0x3ff))) {
                    log_fatal("Code was being ran from IO and manipulated");
                }
            }
#endif
            if ((address & 0x00ff'ffff) > 0x3ff) {
                return ;
            }
            // todo: read pre-calls
            WriteArray<T>(IO, address & 0x3ff, value);
            return;
        case MemoryRegion::PAL:
#ifdef CHECK_INVALID_REFLUSHES
            if constexpr(do_reflush) {
                if (unlikely(NEAR_PC(0x3ff))) {
                    log_fatal("Code was being ran from PAL and manipulated");
                }
            }
#endif
            WriteArray<T>(PAL, address & 0x3ff, value);
            return;
        case MemoryRegion::VRAM:
#ifdef CHECK_INVALID_REFLUSHES
            if constexpr(do_reflush) {
                // this address is actually not quite right, but we are doing this as check anyway
                // I doubt many games will actually run code from VRAM AND manipulate the code right in front of PC
                if (unlikely(NEAR_PC(0xffff))) {
                    log_fatal("Code was being ran from VRAM and manipulated");
                }
            }
#endif
            if ((address & 0x1'ffff) < 0x1'0000) {
                WriteVRAM<T>(address, 0xffff, value);
                WriteArray<T>(VRAM, address & 0xffff, value);
            }
            else {
                // address is already corrected for
                WriteVRAM<T>(0x1'0000 | (address & 0x7fff), 0xffffffff, value);
                WriteArray<T>(VRAM, 0x1'0000 | (address & 0x7fff), value);
            }
            return;
        case MemoryRegion::OAM:
#ifdef CHECK_INVALID_REFLUSHES
            if constexpr(do_reflush) {
                if (unlikely(NEAR_PC(0x3ff))) {
                    log_fatal("Code was being ran from OAM and manipulated");
                }
            }
#endif
            WriteArray<T>(OAM, address & 0x3ff, value);
            return;
        case MemoryRegion::ROM_L1:
        case MemoryRegion::ROM_L2:
        case MemoryRegion::ROM_L:
        case MemoryRegion::ROM_H1:
        case MemoryRegion::ROM_H2:
        case MemoryRegion::ROM_H:
            // todo: EEPROM attempts
            return;
        case MemoryRegion::SRAM:
            log_warn("SRAM write @0x%08x", address);
            return;
        default:
            return;
    }
}