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

Mem::Mem(MMIO* IO, u32* pc_ptr, std::function<void(void)> reflush) {
    this->pc_ptr = pc_ptr;
    this->Reflush = std::move(reflush);

    this->IO = IO;
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

void Mem::FastDMA(u8* start, u16 length) {

}

void Mem::DoDMA(u32 sad, u32 dad, u16 count, u16 control) {
    /*
     * We want to allow faster DMAs in certain memory regions
     * BIOS/unused is a separate case, cause nothing can be read from there anyway, so the DMA can be skipped entirely.
     * I/O we have to be careful, as well as EEPROM/Flash
     * */


}