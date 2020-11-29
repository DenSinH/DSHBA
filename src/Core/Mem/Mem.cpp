#include "Mem.h"

#include "CoreUtils.h"
#include "Backup/BackupDB.h"
#include "NormmattBIOS.h"

#include <filesystem>
#include <string>
#include <fstream>

#include "log.h"

#ifdef DUMP_MEM
#include <fstream>
#endif


size_t LoadFileTo(char* buffer, const std::string& file_name, size_t max_length) {
    std::ifstream infile(file_name, std::ios::binary);

    if (infile.fail()) {
        log_fatal("Failed to open file %s", file_name.c_str());
    }

    infile.seekg(0, std::ios::end);
    size_t length = infile.tellg();
    infile.seekg(0, std::ios::beg);

    if (length > max_length) {
        log_fatal("Failed loading file %s, buffer overflow: %x > %x", file_name.c_str(), length, max_length);
    }

    infile.read(buffer, length);
    log_info("Loaded %x bytes from file %s", length, file_name.c_str());
    return length;
}

/*
 * Reads/writes are always aligned, we handle this in the read handlers
 *
 * the `count` bool in the templates it so we can count cycles conditionally
 * */

Mem::Mem(MMIO* IO, s_scheduler* scheduler, u32* registers_ptr, u32* CPSR_ptr, i32* timer, std::function<void(void)> reflush) {
    this->IO = IO;
    this->Scheduler = scheduler;
    this->registers_ptr = registers_ptr;
    this->pc_ptr = &registers_ptr[15];
    this->CPSR_ptr = CPSR_ptr;
    this->Reflush = std::move(reflush);
    this->timer = timer;

    memset(BIOS, 0, sizeof(BIOS));
    memset(ROM, 0, sizeof(ROM));
    const u8 IdleBranch[5] = "\xfe\xff\xff\xea";
    memcpy(ROM, IdleBranch, 4);
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

    CurrentBIOSReadState = BIOSReadState::StartUp;
    DirtyOAM = true;
    DirtyPAL = true;
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
        delete Backup;
    }

    log_debug("Loading %s", file_path.c_str());
    ROMFile = file_path;
    ROMSize = LoadFileTo(reinterpret_cast<char *>(ROM), file_path, 0x0200'0000);
    SaveFile = file_path.substr(0, file_path.find_last_of('.')) + ".dshba";

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
            log_mem("Detected unspecified EEPROM save type");
            Backup = new EEPROM(UNSPECIFIED_EEPROM_BUS_WIDTH);
            break;
        case BackupType::EEPROM_4:
            log_mem("Detected 4k EEPROM save type");
            Backup = new EEPROM(EEPROM_4K_BUS_WIDTH);
            break;
        case BackupType::EEPROM_64:
            log_mem("Detected 64k EEPROM save type");
            Backup = new EEPROM(EEPROM_64K_BUS_WIDTH);
            break;
        default:
            log_fatal("Unspecified save type");
    }

    if (static_cast<u8>(Type) & static_cast<u8>(BackupType::EEPROM_bit)) {
        SetPageTableEEPROM();
    }
    else {
        SetPageTableNoEEPROM();
    }

    Backup->Load(SaveFile);

    for (size_t addr = ((ROMSize + 3) & ~3); addr < sizeof(ROM); addr += 2) {
        // out of bounds ROM accesses
        // start at next power of 2
        WriteArray<u16>(ROM, addr, addr >> 1);
    }
}

void Mem::LoadBIOS(const std::string& file_path) {
    if (std::filesystem::exists(file_path)) {
        LoadFileTo(reinterpret_cast<char *>(BIOS), file_path, 0x0000'4000);
    }
    else {
        if ((*(u32*)BIOS) == 0) {
            // check if something has been loaded into the BIOS already (reset vector should not be 0)
            log_warn("BIOS file %s does not exist, loading default...", file_path.c_str());
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
            memcpy_s(BIOS, sizeof(BIOS), NormattsBIOS, sizeof(NormattsBIOS));
#else
            memcpy(BIOS, NormattsBIOS, std::min(sizeof(BIOS), sizeof(NormattsBIOS)));
#endif
        }
    }
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

u32 Mem::BusValue() {
    if ((*CPSR_ptr) & 0x0000'0020) {
        // THUMB mode (this is a bit more involved)
        u32 value;
        switch (static_cast<MemoryRegion>((*pc_ptr) >> 24)) {
            case MemoryRegion::eWRAM:
            case MemoryRegion::PAL:
            case MemoryRegion::VRAM:
            case MemoryRegion::ROM_L:
            case MemoryRegion::ROM_L1:
            case MemoryRegion::ROM_L2:
            case MemoryRegion::ROM_H:
            case MemoryRegion::ROM_H1:
            case MemoryRegion::ROM_H2:
                value = Read<u16, false>(*pc_ptr);
                value |= (value << 16);
                return value;
            case MemoryRegion::BIOS:
            case MemoryRegion::OAM:
                if ((*pc_ptr) & 0x3) {
                    // non-4 byte aligned location
                    value  = Read<u16, false>((*pc_ptr) - 2);
                    value |= Read<u16, false>((*pc_ptr)) << 16;
                }
                else {
                    value  = Read<u16, false>((*pc_ptr));
                    value |= Read<u16, false>((*pc_ptr) + 2) << 16;
                }
                return value;
            case MemoryRegion::iWRAM:
            {
                /*
                 * Note: this is not necessarily accurate!
                 * byte load/stores where the base register is already misaligned, but overwritten by the loaded value
                 * will give inaccurate open bus results
                 * */
                // get previous instruction
                u16 instruction = Read<u16, false>((*pc_ptr) - 6);

                // previous prefetched instruction
                value = Read<u16, false>((*pc_ptr) - 2);
                value |= value << 16;

                switch (instruction & 0xf000) {
                    case 0x4000:
                        if (!(instruction & 0x0800)) {
                            break;
                        }
                        // PC-relative load, always 32 bit, value is in Rd
                    case 0x9000:
                        // SP relative load/store
                        // always a word value
                        value = registers_ptr[(instruction >> 8) & 7];
                        break;
                    case 0x5000:
                    {
                        u32 address = registers_ptr[(instruction >> 3) & 0x7] + registers_ptr[(instruction >> 6) & 0x7];
                        if (instruction & 0x0200) {
                            // Load/store with reg offset
                            // load or store doesnt make a difference for the bus value affected
                            if (instruction & 0x0400) {
                                // byte
                                u32 rot = (address & 3) << 3;
                                u32 mask = ~ROTL32(0xff, rot);
                                value = (value & mask) | ROTL32((u8)registers_ptr[instruction & 7], rot);
                            }
                            else {
                                // word
                                value = registers_ptr[instruction & 7];
                            }
                        }
                        else {
                            // load/store sign-extended byte/halfword
                            if (instruction & 0x0800) {
                                // halfword
                                u32 rot = (address & 2) << 3;
                                u32 mask = ~ROTL32(0xffff, rot);
                                value = (value & mask) | ROTL32((u16)registers_ptr[instruction & 7], rot);
                            }
                            else {
                                // byte
                                u32 rot = (address & 3) << 3;
                                u32 mask = ~ROTL32(0xff, rot);
                                value = (value & mask) | ROTL32((u8)registers_ptr[instruction & 7], rot);
                            }
                        }
                        break;
                    }
                    case 0x6000:
                    {
                        // Load/store word with immediate offset
                        // again, loading or storing does not make much of a difference for the bus value
                        value = registers_ptr[instruction & 7];
                        break;
                    }
                    case 0x7000:
                    {
                        // load/store byte with immediate offset
                        u32 address;
                        if (((instruction >> 3) ^ instruction) & 7) {
                            // Rb != Rd
                            address = ((instruction >> 6) & 0x1f); // + registers_ptr[(instruction >> 3) & 0x7];
                        }
                        else {
                            // Rb == Rd, just guess that Rb was word aligned...
                            address = ((instruction >> 6) & 0x1f);
                        }
                        u32 rot = (address & 3) << 3;
                        u32 mask = ~ROTL32(0xff, rot);
                        value = (value & mask) | ROTL32((u8)registers_ptr[instruction & 7], rot);
                        break;
                    }
                    case 0x8000:
                    {
                        // load/store halfword
                        u32 address;
                        if (((instruction >> 3) ^ instruction) & 7) {
                            // Rb != Rd
                            address = (((instruction >> 6) & 0x1f) << 1) + registers_ptr[(instruction >> 3) & 0x7];
                        }
                        else {
                            // Rb == Rd, just guess that Rb was word aligned...
                            address = (((instruction >> 6) & 0x1f) << 1);
                        }
                        u32 rot = (address & 2) << 3;
                        u32 mask = ~ROTL32(0xffff, rot);
                        value = (value & mask) | ROTL32((u16)registers_ptr[instruction & 7], rot);
                        break;
                    }
                    default:
                        // no relevant instruction that affected the bus value
                        break;
                }

                // last prefetched instruction
                if ((*pc_ptr) & 2) {
                    // prefetched value is misaligned
                    value >>= 16;
                    value = (value & 0x0000'ffff) | ((u32)Read<u16, false>(*pc_ptr) << 16);
                }
                else {
                    // prefetched value is aligned
                    value = (value & 0xffff'0000) | (u32)Read<u16, false>(*pc_ptr);
                }

                return value;
            }
            default:
                return 0;
        }
    }

    // last instruction fetched
    if (OpenBusOverrideAt == (*pc_ptr)) {
        OpenBusOverrideAt = 0;
        return OpenBusOverride;
    }

    // last prefetched instruction
    return Read<u32, false>((*pc_ptr) - 4);
}