#pragma once

#include "../IO/MMIO.h"
#include "../IO/IOFlags.h"

#include "default.h"
#include "helpers.h"
#include "flags.h"
#include "const.h"
#include "MemoryHelpers.h"
#include "Backup/BackupMem.h"
#include "Backup/BackupDefault.h"
#include "Backup/SRAM.h"
#include "Backup/Flash.h"
#include "Backup/EEPROM.h"

#include <type_traits>
#include <algorithm>
#include <string>
#include <functional>
#include <utility>
#include <type_traits>

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
    SRAM   = 0x0e,
    OOB,
};

class Mem {
public:
    Mem(MMIO* IO, s_scheduler* scheduler, u32* pc_ptr, u64* timer, std::function<void(void)> reflush);
    ~Mem();

    void Reset();

    template<typename T, bool count> T Read(u32 address);
    template<typename T, bool count, bool do_reflush> void Write(u32 address, T value);

    void LoadROM(const std::string file_path);
    void ReloadROM() {
        LoadROM(ROMFile);
    }
    void LoadBIOS(const std::string& file_path);
    u8* GetPtr(u32 address);
    template<typename T, bool intermittent_events> void DoDMA(s_DMAData* DMA);

    template<typename T>
    static constexpr u8 GetAccessTime(MemoryRegion R) {
        switch (R) {
            case MemoryRegion::BIOS:
                return Mem::AccessTiming<T, MemoryRegion::BIOS>();
            case MemoryRegion::iWRAM:
                return Mem::AccessTiming<T, MemoryRegion::iWRAM>();
            case MemoryRegion::eWRAM:
                return Mem::AccessTiming<T, MemoryRegion::eWRAM>();
            case MemoryRegion::IO:
                return Mem::AccessTiming<T, MemoryRegion::IO>();
            case MemoryRegion::PAL:
                return Mem::AccessTiming<T, MemoryRegion::PAL>();
            case MemoryRegion::VRAM:
                return Mem::AccessTiming<T, MemoryRegion::VRAM>();
            case MemoryRegion::OAM:
                return Mem::AccessTiming<T, MemoryRegion::OAM>();
            case MemoryRegion::ROM_L:
                return Mem::AccessTiming<T, MemoryRegion::ROM_L>();
            case MemoryRegion::ROM_L1:
                return Mem::AccessTiming<T, MemoryRegion::ROM_L1>();
            case MemoryRegion::ROM_L2:
                return Mem::AccessTiming<T, MemoryRegion::ROM_L2>();
            case MemoryRegion::ROM_H:
                return Mem::AccessTiming<T, MemoryRegion::ROM_H>();
            case MemoryRegion::ROM_H1:
                return Mem::AccessTiming<T, MemoryRegion::ROM_H1>();
            case MemoryRegion::ROM_H2:
                return Mem::AccessTiming<T, MemoryRegion::ROM_H2>();
            case MemoryRegion::SRAM:
                return Mem::AccessTiming<T, MemoryRegion::SRAM>();
            case MemoryRegion::Unused:
                return Mem::AccessTiming<T, MemoryRegion::Unused>();
            case MemoryRegion::OOB:
                return Mem::AccessTiming<T, MemoryRegion::OOB>();
            default:
                return 1;
        }
    }

    // timings for ROM are different
    template<typename T, MemoryRegion R>
    static constexpr u8 AccessTiming() {
        switch (R) {
            case MemoryRegion::BIOS:
            case MemoryRegion::iWRAM:
            case MemoryRegion::IO:
            case MemoryRegion::OAM:  // todo: check if VBlank/HBlank for extra cycle
                return 1;
            case MemoryRegion::eWRAM:
                if constexpr(std::is_same_v<T, u32>) {
                    return 6;
                }
                return 3;
            case MemoryRegion::PAL:
            case MemoryRegion::VRAM:
                // todo: check if VBlank/HBlank for extra cycle
                if constexpr(std::is_same_v<T, u32>) {
                    return 2;
                }
                return 1;
            case MemoryRegion::ROM_L:
            case MemoryRegion::ROM_H:
                if constexpr(std::is_same_v<T, u32>) {
                    return 5;
                }
                return 2;
            case MemoryRegion::ROM_L1:
            case MemoryRegion::ROM_H1:
                if constexpr(std::is_same_v<T, u32>) {
                    return 9;
                }
                return 5;
            case MemoryRegion::ROM_L2:
            case MemoryRegion::ROM_H2:
                // todo: waitstates/sequential accesses
                if constexpr(std::is_same_v<T, u32>) {
                    return 17;
                }
                return 9;
            case MemoryRegion::SRAM:
                return 5;
            case MemoryRegion::Unused:
            case MemoryRegion::OOB:
            default:
                // todo: ?
                return 1;
        }
    }

private:
    friend class GBAPPU;   // allow VMEM to be buffered
    friend class MEM_INL;  // to cheese IDEs
    friend class Initializer;

    template<typename T> ALWAYS_INLINE void WriteVRAM(u32 address, T value);
    static ALWAYS_INLINE constexpr u32 MaskVRAMAddress(const u32 address) {
        if ((address & 0x1'ffff) < 0x1'0000) {
            return address & 0xffff;
        }
        return 0x1'0000 | (address & 0x7fff);
    }

    // assumes save type is actually EEPROM
    [[nodiscard]] ALWAYS_INLINE constexpr bool IsEEPROMAccess(const u32 address) const {
        ASSUME((address >= 0x0800'0000));
        return ((address & 0x01ff'ffff) > 0x01ff'feff) || ((ROMSize <= 0x0100'0000) && (address >= 0x0d00'0000));
    }

    s_scheduler* Scheduler;
    u32* pc_ptr;
    u64* timer;
    std::function<void(void)> Reflush;
    s_UpdateRange VRAMUpdate = { .min=sizeof(VRAMMEM), .max=0 };
    bool DirtyOAM     = false;
    bool PrevDirtyOAM = false;  // OAM update delay

    u8 BIOS  [0x4000]        = {};
    u8 eWRAM [0x4'0000]      = {};
    u8 iWRAM [0x8000]        = {};
    MMIO* IO;
    PALMEM PAL               = {};
    VRAMMEM VRAM             = {};
    OAMMEM OAM               = {};
    u8 ROM   [0x0200'0000]   = {};
    BackupMem* Backup        = nullptr;
    BackupType Type = BackupType::SRAM;
    size_t ROMSize = 0;

    BackupType FindBackupType();

    std::string ROMFile;
    std::string SaveFile;

    s_event DumpSave;
    static SCHEDULER_EVENT(DumpSaveEvent);

#define INLINED_INCLUDES
#include "MemDMAUtil.inl"
#undef INLINED_INCLUDES

#ifdef FAST_DMA
    template<typename T, bool intermittent_events> void FastDMA(s_DMAData* DMA);    // incrementing DMAs in both directions
    template<typename T, bool intermittent_events> void MediumDMA(s_DMAData* DMA);  // DMAs from and to safe memory regions
#endif
    template<typename T, bool intermittent_events> void SlowDMA(s_DMAData* DMA);    // DMAs with wrapping/special behavior
};

#include "MemReadWrite.inl" // Read/Write related templated functions
#include "MemDMA.inl"       // DMA related templated functions