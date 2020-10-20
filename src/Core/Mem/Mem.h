#pragma once

#include "default.h"
#include "MemoryHelpers.h"

#include <string>

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
        Mem();
        ~Mem();

        template<typename T, bool count> T Read(u32 address);
        template<typename T, bool count> void Write(u32 address, T value);
        
        void LoadROM(const std::string& file_path);
        void LoadBIOS(const std::string& file_path);

    private:
        u8 BIOS  [0x4000];
        u8 eWRAM [0x4'0000];
        u8 iWRAM [0x8000];
        u8 PAL   [0x400];
        u8 VRAM  [0x1'8000];
        u8 OAM   [0x400];
        u8 ROM   [0x0200'0000];
};

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
            log_warn("IO read @0x%08x", address);
            return 0;
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

template<typename T, bool count>
void Mem::Write(u32 address, T value) {
    switch (static_cast<MemoryRegion>(address >> 24)) {
        case MemoryRegion::BIOS:
            // BIOS is read-only
        case MemoryRegion::Unused:
            return;
        case MemoryRegion::eWRAM:
            WriteArray<T>(eWRAM, address & 0x3'ffff, value);
            return;
        case MemoryRegion::iWRAM:
            WriteArray<T>(iWRAM, address & 0x7fff, value);
            return;
        case MemoryRegion::IO:
            log_warn("IO write @0x%08x", address);
            return;
        case MemoryRegion::PAL:
            WriteArray<T>(PAL, address & 0x3ff, value);
            return;
        case MemoryRegion::VRAM:
            if ((address & 0x1'ffff) < 0x1'0000) {
                WriteArray<T>(VRAM, address & 0xffff, value);
            }
            WriteArray<T>(VRAM, 0x1'0000 | (address & 0x7fff), value);
            return;
        case MemoryRegion::OAM:
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