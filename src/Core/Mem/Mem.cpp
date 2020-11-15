#include "Mem.h"

#include "CoreUtils.h"
#include "Backup/BackupDB.h"

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
    memset(ROM, 0, sizeof(ROM));
    Reset();

    DumpSave = (s_event) {
        .callback = DumpSaveEvent,
        .caller = this
    };
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

void Mem::Reset() {
//    memset(BIOS, 0, sizeof(BIOS));
    memset(eWRAM, 0, sizeof(eWRAM));
    memset(iWRAM, 0, sizeof(iWRAM));
    memset(PAL, 0, sizeof(PALMEM));
    memset(VRAM, 0, sizeof(VRAMMEM));
    memset(OAM, 0, sizeof(OAMMEM));

    DirtyOAM = true;
    VRAMUpdate = { .min=0, .max=sizeof(VRAMMEM) };
}

static std::map<BackupType, const char*> BackupSearchString = {
        { BackupType::SRAM,      "SRAM_V" },
        { BackupType::FLASH,     "FLASH_V" },
        { BackupType::FLASH_64,  "FLASH512_V" },
        { BackupType::FLASH_128, "FLASH1M_V" },
        { BackupType::EEPROM,    "EEPROM_V" },
};

bool FindInROM(const u8* ROM, u32 ROMSize, BackupType Type) {
    const char* SearchString = BackupSearchString[Type];
    log_debug("Searching for %s", SearchString);
    for (u32 i = 0; i < ROMSize; i++) {
        bool match = true;
        for (u32 j = 0; SearchString[j] != 0; j++) {
            if (i + j >= ROMSize) {
                match = false;
                break;
            }

            if (ROM[i + j] != SearchString[j]) {
                match = false;
                break;
            }
        }
        if (match) {
            return true;
        }
    }
    return false;
}

BackupType Mem::FindBackupType() {
    char GameCode[5];
    memcpy(GameCode, &ROM[0xac], 4);
    GameCode[4] = '\0';

    const std::string ID = std::string(GameCode);
    auto it = GameDB.find(ID);
    if (it != GameDB.cend()) {
        s_SaveInfo SaveInfo = it->second;

        if (SaveInfo.Type != BackupType::Detect) {
            log_mem("ROM in game database");
            return SaveInfo.Type;
        }
    }

    // not in DB, try matching save type substrings
    for (auto type : { BackupType::EEPROM, BackupType::FLASH_128, BackupType::FLASH_64, BackupType::FLASH }) {
        if (FindInROM(ROM, ROMSize, type)) {
            return type;
        }
    }

    // couldn't find anything, use SRAM as default
    return BackupType::SRAM;
}

void Mem::LoadROM(const std::string file_path) {
    // dump old save
    if (Backup) {
        Backup->Dump(SaveFile);
        free(Backup);
    }

    log_debug("Loading %s", file_path.c_str());
    ROMFile = file_path;
    ROMSize = LoadFileTo(reinterpret_cast<char *>(ROM), file_path, 0x0200'0000);
    SaveFile = file_path.substr(0, file_path.find_last_of('.')) + ".dshba";

    // todo: check backup type
    Type = FindBackupType();

    switch (Type) {
        case BackupType::SRAM:
            log_mem("Detected SRAM save type");
            Backup = new SRAM();
            break;
        case BackupType::FLASH:
        case BackupType::FLASH_128:
            log_mem("Detected FLASH1M save type");
            // just in case for unspecified FLASH type we use 2 banks
            Backup = new Flash(true);
            break;
        case BackupType::FLASH_64:
            log_mem("Detected FLASH512K save type");
            Backup = new Flash(false);
            break;
        case BackupType::EEPROM:
        case BackupType::EEPROM_4:
        case BackupType::EEPROM_64:
            log_mem("Detected EEPROM save type");
            // todo
            Backup = new SRAM();
            break;
        default:
            log_fatal("Unspecified save type");
    }
    Backup->Load(SaveFile);

    for (size_t addr = ROMSize; addr < sizeof(ROM); addr += 2) {
        // out of bounds ROM accesses
        WriteArray<u16>(ROM, addr, addr >> 1);
    }
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

SCHEDULER_EVENT(Mem::DumpSaveEvent) {
    auto Memory = (Mem*)caller;

    if (--Memory->Backup->Dirty > 0) {
        // reschedule to check again
        scheduler->AddEventAfter(event, CYCLES_PER_FRAME);
    }
    else {
        Memory->Backup->Dump(Memory->SaveFile);
    }
}