#pragma once

#include "BackupMem.h"

class SRAM : public BackupMem {

public:
    SRAM();

private:
    u8 _storage[0x8000];

    void Dump(const std::string &file_name) override;
    virtual void Load(const std::string& file_name) override;
    virtual u8 Read(u32 address) override;
    virtual void Write(u32 address, u8 value) override;
};
