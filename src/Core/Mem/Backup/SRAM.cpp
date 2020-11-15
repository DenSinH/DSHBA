#include "SRAM.h"

#include "log.h"

#include <fstream>

SRAM::SRAM() {

}

void SRAM::Dump(const std::string& file_name) {
    std::ofstream file(file_name, std::ios::trunc | std::ios::binary);

    if (file.is_open()) {
        file.write(reinterpret_cast<const char *>(_storage), sizeof(_storage));
        file.close();
    }
    else {
        log_warn("Opening save file failed!");
    }
}

void SRAM::Load(const std::string& file_name) {
    std::ifstream file(file_name, std::ios::binary);

    if (file.is_open()) {
        file.read(reinterpret_cast<char *>(_storage), sizeof(_storage));
        file.close();
    }
    else {
        // don't warn, file might not have been created for this game
    }
}

u8 SRAM::Read(u32 address) {
    return _storage[address & 0x7fff];
}

void SRAM::Write(u32 address, u8 value) {
    _storage[address & 0x7fff] = value;
}