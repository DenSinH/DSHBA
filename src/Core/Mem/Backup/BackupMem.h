#pragma once

#include "default.h"

#include <string>
#include <string.h>

class BackupMem {
public:
    // set to some int, then counted down, once it hits 0, dump it
    static constexpr const int MaxDirtyChecks = 15;  // frames
    int Dirty;

    virtual void Dump(const std::string& file_name) {};
    virtual void Load(const std::string& file_name) {};
    virtual u8 Read(u32 address) { return 0; };
    virtual void Write(u32 address, u8 value) {};
};