#include "Mem.h"

#include "CoreUtils.h"


/*
 * Reads/writes are always aligned, we handle this in the read handlers
 *
 * the `count` bool in the templates it so we can count cycles conditionally
 * */

Mem::Mem() {
    memset(BIOS, 0, sizeof(BIOS));
    memset(eWRAM, 0, sizeof(eWRAM));
    memset(iWRAM, 0, sizeof(iWRAM));
    memset(&VMEM, 0, sizeof(s_VMEM));
    memset(ROM, 0, sizeof(ROM));
}

Mem::~Mem() {

}

void Mem::LoadROM(const std::string& file_path) {
    LoadFileTo(reinterpret_cast<char *>(ROM), file_path, 0x0200'0000);
}

void Mem::LoadBIOS(const std::string& file_path) {
    LoadFileTo(reinterpret_cast<char *>(BIOS), file_path, 0x0000'4000);
}