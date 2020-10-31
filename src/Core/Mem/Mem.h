#pragma once

#include "../IO/MMIO.h"

#include "default.h"
#include "helpers.h"
#include "flags.h"
#include "MemoryHelpers.h"

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

class Mem {
public:
    Mem(MMIO* IO, s_scheduler* scheduler, u32* pc_ptr, u64* timer, std::function<void(void)> reflush);
    ~Mem();

    template<typename T, bool count> T Read(u32 address);
    template<typename T, bool count, bool do_reflush> void Write(u32 address, T value);

    void LoadROM(const std::string& file_path);
    void LoadBIOS(const std::string& file_path);
    u8* GetPtr(u32 address);
    template<typename T, bool intermittent_events> void DoDMA(s_DMAData* DMA);

private:
    friend class GBAPPU;  // allow VMEM to be buffered
    friend class Initializer;

    template<typename T> ALWAYS_INLINE void WriteVRAM(u32 address, T value);

    s_scheduler* Scheduler;
    u32* pc_ptr;
    u64* timer;
    std::function<void(void)> Reflush;
    s_UpdateRange VRAMUpdate = { .min=sizeof(VRAMMEM), .max=0 };

    u8 BIOS  [0x4000]        = {};
    u8 eWRAM [0x4'0000]      = {};
    u8 iWRAM [0x8000]        = {};
    MMIO* IO;
    PALMEM PAL               = {};
    VRAMMEM VRAM             = {};
    OAMMEM OAM               = {};
    u8 ROM   [0x0200'0000]   = {};

    template<typename T, bool intermittent_events> void FastDMA(s_DMAData* DMA);
    template<typename T, bool intermittent_events> void SlowDMA(s_DMAData* DMA);
};

static ALWAYS_INLINE constexpr u32 MaskVRAMAddress(const u32 address) {
    if ((address & 0x1'ffff) < 0x1'0000) {
        return address & 0xffff;
    }
    return 0x1'0000 | (address & 0x7fff);
}

#include "DMA.inl"

template<typename T, bool count>
T Mem::Read(u32 address) {
    if constexpr(count) {
        // todo: actual timings
        (*timer)++;
    }

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
            return IO->Read<T>(address & 0x3ff);
        case MemoryRegion::PAL:
            return ReadArray<T>(PAL, address & 0x3ff);
        case MemoryRegion::VRAM:
            return ReadArray<T>(VRAM, MaskVRAMAddress(address));
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
ALWAYS_INLINE void Mem::WriteVRAM(u32 address, T value) {
    // on writes, I guess it's actually pretty likely the old value gets overwritten
    if (likely(ReadArray<T>(VRAM, address) != value)) {
        if ((address + sizeof(T)) > VRAMUpdate.max) {
            VRAMUpdate.max = address + sizeof(T);
        }
        if (address < VRAMUpdate.min) {
            VRAMUpdate.min = address;
        }
    }
}

// We only want to re-flush the ARM7TDMI pipeline if we are in an instruction when it happens
// DMAs, we assume that PC is not in the DMA-ed part of the code
#define NEAR_PC(mask) ((address >> 24) == (*pc_ptr >> 24)) && ((std::abs(int(address - *pc_ptr)) & (mask)) <= 8)
template<typename T, bool count, bool do_reflush>
void Mem::Write(u32 address, T value) {
    if constexpr(count) {
        // todo: actual timings
        (*timer)++;
    }

    switch (static_cast<MemoryRegion>(address >> 24)) {
        case MemoryRegion::BIOS:
            // BIOS is read-only
        case MemoryRegion::Unused:
            return;
        case MemoryRegion::eWRAM:
            if constexpr(do_reflush) {
                if (unlikely(NEAR_PC(0x3'ffff))) {
//                    log_debug("%x near %x, distance %x", address >> 24, *pc_ptr >> 24, std::abs(int(address - *pc_ptr)) & (0x3'ffff));
                    Reflush();
                }
            }

            WriteArray<T>(eWRAM, address & 0x3'ffff, value);
            return;
        case MemoryRegion::iWRAM:
            if constexpr(do_reflush) {
                if (unlikely(NEAR_PC(0x7fff))) {
//                    log_debug("%x near %x, distance %x", address >> 24, *pc_ptr >> 24, std::abs(int(address - *pc_ptr)) & (0x3'ffff));
                    Reflush();
                }
            }

            WriteArray<T>(iWRAM, address & 0x7fff, value);
            return;
        case MemoryRegion::IO:
#ifdef CHECK_INVALID_REFLUSHES
            if constexpr(do_reflush) {
                if (unlikely(NEAR_PC(0x3ff))) {
                    log_warn("Code was being ran from MMIO and manipulated");
                    Reflush();
                }
            }
#endif
            IO->Write<T>(address & 0x3ff, value);
            return;
        case MemoryRegion::PAL:
#ifdef CHECK_INVALID_REFLUSHES
            if constexpr(do_reflush) {
                if (unlikely(NEAR_PC(0x3ff))) {
                    log_warn("Code was being ran from PAL and manipulated");
                    Reflush();
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
                    log_warn("Code was being ran from VRAM and manipulated");
                    Reflush();
                }
            }
#endif

            WriteVRAM<T>(MaskVRAMAddress(address), value);
            WriteArray<T>(VRAM, MaskVRAMAddress(address), value);
            return;
        case MemoryRegion::OAM:
#ifdef CHECK_INVALID_REFLUSHES
            if constexpr(do_reflush) {
                if (unlikely(NEAR_PC(0x3ff))) {
                    log_warn("Code was being ran from OAM and manipulated");
                    Reflush();
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