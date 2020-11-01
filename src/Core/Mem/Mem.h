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

#ifdef FAST_DMA
    template<typename T, bool intermittent_events> void FastDMA(s_DMAData* DMA);  // incrementing DMAs in both directions
    template<typename T, bool intermittent_events> void MediumDMA(s_DMAData* DMA);  // DMAs from and to safe memory regions
#endif
    template<typename T, bool intermittent_events> void SlowDMA(s_DMAData* DMA);  // DMAs with wrapping/special behavior
};

static ALWAYS_INLINE constexpr u32 MaskVRAMAddress(const u32 address) {
    if ((address & 0x1'ffff) < 0x1'0000) {
        return address & 0xffff;
    }
    return 0x1'0000 | (address & 0x7fff);
}

#include "MemReadWrite.inl" // Read/Write related templated functions
#include "MemDMA.inl"       // DMA related templated functions