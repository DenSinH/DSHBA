#include "Mem.h"

#include "CoreUtils.h"

#ifdef DUMP_MEM
#include <fstream>
#endif

/*
 * Reads/writes are always aligned, we handle this in the read handlers
 *
 * the `count` bool in the templates it so we can count cycles conditionally
 * */

Mem::Mem(MMIO* IO, s_scheduler* scheduler, u32* pc_ptr, u64* timer, std::function<void(void)> reflush) {
    this->IO = IO;
    this->Scheduler = scheduler;
    this->pc_ptr = pc_ptr;
    this->Reflush = std::move(reflush);
    this->timer = timer;

    memset(BIOS, 0, sizeof(BIOS));
    memset(eWRAM, 0, sizeof(eWRAM));
    memset(iWRAM, 0, sizeof(iWRAM));
    memset(PAL, 0, sizeof(PALMEM));
    memset(VRAM, 0, sizeof(VRAMMEM));
    memset(OAM, 0, sizeof(OAMMEM));
    memset(ROM, 0, sizeof(ROM));
}

Mem::~Mem() {

#ifdef DUMP_MEM
    std::ofstream file;
    log_debug("Dumping memory region " str(DUMP_MEM) "...");

    file.open("./files/" str(DUMP_MEM) , std::fstream::binary);

    if (!file) {
        log_warn("Could not open dump file");
    }
    else {
        file.write((char*)DUMP_MEM, sizeof(DUMP_MEM));
        file.close();
    }
#endif
}

void Mem::LoadROM(const std::string& file_path) {
    LoadFileTo(reinterpret_cast<char *>(ROM), file_path, 0x0200'0000);
}

void Mem::LoadBIOS(const std::string& file_path) {
    LoadFileTo(reinterpret_cast<char *>(BIOS), file_path, 0x0000'4000);
}

u8* Mem::GetPtr(u32 address) {
    switch (static_cast<MemoryRegion>(address >> 24)) {
        case MemoryRegion::BIOS:
            return &BIOS[address & 0x3fff];
        case MemoryRegion::Unused:
            return nullptr;
        case MemoryRegion::eWRAM:
            return &eWRAM[address & 0x3'ffff];
        case MemoryRegion::iWRAM:
            return &iWRAM[address & 0x7fff];
        case MemoryRegion::IO:
            return &IO->Registers[address & 0x3ff];
        case MemoryRegion::PAL:
            return &PAL[address & 0x3ff];
        case MemoryRegion::VRAM:
            return &VRAM[MaskVRAMAddress(address)];
        case MemoryRegion::OAM:
            return &OAM[address & 0x3ff];
        case MemoryRegion::ROM_L1:
        case MemoryRegion::ROM_L2:
        case MemoryRegion::ROM_L:
        case MemoryRegion::ROM_H1:
        case MemoryRegion::ROM_H2:
        case MemoryRegion::ROM_H:
            return &ROM[address & 0x01ff'ffff];
        case MemoryRegion::SRAM:
        default:
            return nullptr;
    }
}