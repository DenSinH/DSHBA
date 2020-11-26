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

enum class MemoryRegion : u8 {
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
};

enum class BIOSReadState : u32 {
    StartUp   = 0xe129f000,
    DuringIRQ = 0xe25ef004,
    AfterIRQ  = 0xe55ec002,
    AfterSWI  = 0xe3a02004,
};

class Mem {
public:
    Mem(MMIO* IO, s_scheduler* scheduler, u32* registers_ptr, u32* CPSR_ptr, i32* timer, std::function<void(void)> reflush);
    ~Mem();

    u32 OpenBusOverride = 0;
    u32 OpenBusOverrideAt = 0;

    BIOSReadState CurrentBIOSReadState = BIOSReadState::StartUp;

    void Reset();

    template<typename T, bool count, bool in_dma = false> ALWAYS_INLINE T Read(u32 address);

    template<typename T, bool count, bool do_reflush> void Write(u32 address, T value);

    void LoadROM(const std::string file_path);
    void ReloadROM() {
        LoadROM(ROMFile);
    }
    void LoadBIOS(const std::string& file_path);
    u8* GetPtr(u32 address);
    template<typename T, bool intermittent_events> void DoDMA(s_DMAData* DMA);

    template<typename T>
    [[nodiscard]] ALWAYS_INLINE u8 GetAccessTime(MemoryRegion R) const {
        if constexpr(std::is_same_v<T, u32>) {
            return WordAccessTime[static_cast<u8>(R)];
        }
        else {
            return SubWordAccessTime[static_cast<u8>(R)];
        }
    }

private:
    friend class GBAPPU;   // allow VMEM to be buffered
    friend class Mem_INL;  // to cheese IDEs
    friend class Initializer;

    template<typename T, bool in_dma = false> T RawReadSlow(u32 address);

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

    template<typename T> ALWAYS_INLINE T ReadDMALatch(u32 address) {
        if constexpr(std::is_same_v<T, u16>) {
            return DMALatch >> ((address & 2) << 2);  // alignment
        }
        else {
            return DMALatch;
        }
    }

    s_scheduler* Scheduler;
    u32* pc_ptr;
    u32* registers_ptr;
    u32* CPSR_ptr;
    i32* timer;
    std::function<void(void)> Reflush;
    s_UpdateRange VRAMUpdate = { .min=sizeof(VRAMMEM), .max=0 };
    bool DirtyOAM     = false;
    bool PrevDirtyOAM = false;  // OAM update delay
    bool DirtyPAL     = false;

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
    u32 BusValue();

    std::string ROMFile;
    std::string SaveFile;

    s_event DumpSave;
    static SCHEDULER_EVENT(DumpSaveEvent);

    u32 DMALatch = 0;

    std::array<u8, 255> SubWordAccessTime = [] {
        std::array<u8, 255> table = {};

        // fill with 1s
        for (u8 & i : table) {
            i = 1;
        }

        table[static_cast<u8>(MemoryRegion::eWRAM)]  = 3;

        table[static_cast<u8>(MemoryRegion::ROM_L)]  = 2;
        table[static_cast<u8>(MemoryRegion::ROM_H)]  = 2;

        table[static_cast<u8>(MemoryRegion::ROM_L1)] = 5;
        table[static_cast<u8>(MemoryRegion::ROM_H1)] = 5;

        table[static_cast<u8>(MemoryRegion::ROM_L2)] = 9;
        table[static_cast<u8>(MemoryRegion::ROM_H2)] = 9;
        
        table[static_cast<u8>(MemoryRegion::SRAM)]   = 5;

        return table;
    }();

    std::array<u8, 255> WordAccessTime = [] {
        std::array<u8, 255> table = {};

        // fill with 1s
        for (u8 & i : table) {
            i = 1;
        }

        table[static_cast<u8>(MemoryRegion::eWRAM)]  = 6;
        table[static_cast<u8>(MemoryRegion::PAL)]    = 2;
        table[static_cast<u8>(MemoryRegion::VRAM)]   = 2;

        table[static_cast<u8>(MemoryRegion::ROM_L)]  = 6;
        table[static_cast<u8>(MemoryRegion::ROM_H)]  = 6;

        table[static_cast<u8>(MemoryRegion::ROM_L1)] = 8;
        table[static_cast<u8>(MemoryRegion::ROM_H1)] = 8;

        table[static_cast<u8>(MemoryRegion::ROM_L2)] = 12;
        table[static_cast<u8>(MemoryRegion::ROM_H2)] = 12;

        table[static_cast<u8>(MemoryRegion::SRAM)]   = 5;

        return table;
    }();

    std::array<u32, 255> ReflushMask = [] {
        std::array<u32, 255> table = {};

        table[static_cast<u8>(MemoryRegion::eWRAM)] = 0x3ffff;
        table[static_cast<u8>(MemoryRegion::iWRAM)] = 0x7fff;
        table[static_cast<u8>(MemoryRegion::IO)]    = 0x3ff;
        table[static_cast<u8>(MemoryRegion::PAL)]   = 0x3ff;
        table[static_cast<u8>(MemoryRegion::VRAM)]  = 0x7fff;
        table[static_cast<u8>(MemoryRegion::OAM)]   = 0x3ff;

        return table;
    }();

#define INLINED_INCLUDES
#include "MemDMAUtil.inl"
#include "MemPageTables.inl"
#undef INLINED_INCLUDES

#ifdef FAST_DMA
    template<typename T, bool intermittent_events> void FastDMA(s_DMAData* DMA);    // incrementing DMAs in both directions
    template<typename T, bool intermittent_events> void MediumDMA(s_DMAData* DMA);  // DMAs from and to safe memory regions
#endif
    template<typename T, bool intermittent_events> void SlowDMA(s_DMAData* DMA);    // DMAs with wrapping/special behavior
};

#include "MemReadWrite.inl" // Read/Write related templated functions
#include "MemDMA.inl"       // DMA related templated functions