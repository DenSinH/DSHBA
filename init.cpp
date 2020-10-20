#include "init.h"
#include "src/Frontend/interface.h"

#include "default.h"

#include <memory>

static GBA* gba;

static u8 ReadByte(u64 offset) {
    return gba->memory.Read<u8, false>(offset);
}

u8* ValidAddressMask(u32 address) {
    // all memory can be read for the GBA
    switch (static_cast<MemoryRegion>(address >> 24)) {
        case MemoryRegion::BIOS:
            return &gba->memory.BIOS[address & 0x3fff];
        case MemoryRegion::Unused:
            return nullptr;
        case MemoryRegion::eWRAM:
            return &gba->memory.eWRAM[address & 0x3'ffff];
        case MemoryRegion::iWRAM:
            return &gba->memory.iWRAM[address & 0x7fff];
        case MemoryRegion::IO:
            return nullptr;
        case MemoryRegion::PAL:
            return &gba->memory.PAL[address & 0x3ff];
        case MemoryRegion::VRAM:
            if ((address & 0x1'ffff) < 0x1'0000) {
                return &gba->memory.VRAM[address & 0xffff];
            }
            return &gba->memory.VRAM[0x1'0000 | (address & 0x7fff)];
        case MemoryRegion::OAM:
            return &gba->memory.OAM[address & 0x3ff];
        case MemoryRegion::ROM_L1:
        case MemoryRegion::ROM_L2:
        case MemoryRegion::ROM_L:
        case MemoryRegion::ROM_H1:
        case MemoryRegion::ROM_H2:
        case MemoryRegion::ROM_H:
            return &gba->memory.ROM[address & 0x01ff'ffff];
        case MemoryRegion::SRAM:
            return nullptr;
        default:
            return nullptr;
    }
}

GBA* init() {
    gba = (GBA*)malloc(sizeof(GBA));
    new(gba) GBA;

    frontend_init(
            &gba->shutdown,
            &gba->cpu.pc,
            0x1'0000'0000ULL,
            ValidAddressMask,
            ReadByte,
            nullptr
    );

    char label[0x100];

    int cpu_tab = add_register_tab("ARM7TDMI");

    for (int i = 0; i < 13; i++) {
        sprintf(label, "r%d", i);
        add_register_data(label, &gba->cpu.registers[i], 4, cpu_tab);
    }

    add_register_data("r13 (SP)", &gba->cpu.sp, 4, cpu_tab);
    add_register_data("r14 (LR)", &gba->cpu.lr, 4, cpu_tab);
    add_register_data("r15 (PC)", &gba->cpu.pc, 4, cpu_tab);

    return gba;
}
