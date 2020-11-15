#pragma once

enum class BackupType {
    SRAM,
    FLASH,
    FLASH_64,
    FLASH_128,
    EEPROM,
    EEPROM_4,
    EEPROM_64,
    Detect,
};

enum class GPIODeviceType {
    None,
    RTC
};