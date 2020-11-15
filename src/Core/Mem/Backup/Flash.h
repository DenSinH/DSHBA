#pragma once

#include "BackupMem.h"

class Flash : public BackupMem {

public:
    Flash(bool megabit);

private:
    bool Megabit;
    u8 _banks[2][0x10000];
    static constexpr const u8 SanyoManufacturerID = 0x62;
    static constexpr const u8 SanyoDeviceID = 0x13;

    enum class WriteState {
        CommandStart1,
        CommandStart2,
        Command
    };

    enum class ExpectState {
        Nothing = 0,
        Erase,
        Byte,
        BankSwitch,
    };

    u8 Bank = 0;

    ExpectState Expect = ExpectState::Nothing;
    WriteState State   = WriteState::CommandStart1;
    bool IDMode        = false;

    void Dump(const std::string &file_name) override;
    void Load(const std::string& file_name) override;
    u8 Read(u32 address) override;
    void Write(u32 address, u8 value) override;

    void Erase(u32 start, size_t size) {
        memset(&_banks[Bank][start], 0xff, size);
    }

    void EraseAll() {
        memset(_banks, 0xff, sizeof(_banks));
    }
};
