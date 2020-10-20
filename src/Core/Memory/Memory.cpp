#include "Memory.h"

#include "CoreUtils.h"


/*
 * Reads/writes are always aligned, we handle this in the read handlers
 *
 * the `count` bool in the templates it so we can count cycles conditionally
 * */

void Memory::LoadROM(const std::string& file_path) {
    LoadFileTo(reinterpret_cast<char *>(ROM), file_path, 0x0200'0000);
}

void Memory::LoadBIOS(const std::string& file_path) {
    LoadFileTo(reinterpret_cast<char *>(BIOS), file_path, 0x0000'4000);
}