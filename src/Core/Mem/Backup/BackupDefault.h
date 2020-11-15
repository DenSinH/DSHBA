#pragma once

enum class BackupType : u8 {
    SRAM       = 0x00,

    FLASH      = 0x01,
    FLASH_64   = 0x02,
    FLASH_128  = 0x03,

    EEPROM_bit = 0x04,
    EEPROM     = 0x04,
    EEPROM_4   = 0x05,
    EEPROM_64  = 0x06,

    Detect     = 0x80,
};

enum class GPIODeviceType {
    None,
    RTC
};