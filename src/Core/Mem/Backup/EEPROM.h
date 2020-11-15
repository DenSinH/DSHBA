#pragma once

#include "BackupMem.h"

#define UNSPECIFIED_EEPROM_BUS_WIDTH 0
#define EEPROM_4K_BUS_WIDTH 6
#define EEPROM_64K_BUS_WIDTH 14

class EEPROM : public BackupMem {

public:
    EEPROM(u32 bus_width);
    // either 0 (uninitialized), 6 or 14
    // if it is 0, to be set by the Mem class holding this backup region to the appropriate value whenever DMA 3 is
    // started to the backup region for the first time
    u32 BusWidth;

    ALWAYS_INLINE void SetBusWidth(u32 bus_width) {
        BusWidth = bus_width;
        if (bus_width == EEPROM_4K_BUS_WIDTH) {
            Size = 0x200;
        }
        else {
            Size = 0x2000;
        }
    }

private:
    u32 Size;
    u8 _storage[0x2000];

    enum class AccessType : u8 {
        Read  = 0b11,
        Write = 0b10,
    };

    const static u32 ReadBitCounterReset = 68;

    u32 ReadAddress = 0, WriteAddress = 0;
    u32 Buffer = 0;
    AccessType Access;
    u32 WriteBitCounter = 0;
    u32 ReadBitCounter = 0;

    void Dump(const std::string &file_name) override;
    void Load(const std::string& file_name) override;
    u8 Read(u32 address) override;
    void Write(u32 address, u8 value) override;
};

