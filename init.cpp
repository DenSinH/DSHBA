#include "init.h"

#include "default.h"
#include "debugging.h"

#include <memory>

static GBA* gba;

CONSOLE_COMMAND(Initializer::pause_system) {
#ifdef DO_DEBUGGER
    gba->paused = true;

    STRCPY(output, MAX_OUTPUT_LENGTH, "System paused");
#endif
}

CONSOLE_COMMAND(Initializer::unpause_system) {
#ifdef DO_DEBUGGER
    gba->paused = false;

    STRCPY(output, MAX_OUTPUT_LENGTH, "System unpaused");
#endif
}

CONSOLE_COMMAND(Initializer::break_system) {
#ifdef DO_DEBUGGER
    if (argc < 2) {
        pause_system(args, argc, output);
        return;
    }

    u32 breakpoint = parsehex(args[1]);
    add_breakpoint(&gba->breakpoints, breakpoint);
    SPRINTF(output, MAX_OUTPUT_LENGTH, "Added breakpoint at %08x", breakpoint);
#endif
}

CONSOLE_COMMAND(Initializer::unbreak_system) {
#ifdef DO_DEBUGGER
    if (argc < 2) {
        unpause_system(args, argc, output);
        return;
    }

    u32 breakpoint = parsehex(args[1]);
    remove_breakpoint(&gba->breakpoints, breakpoint);
    SPRINTF(output, MAX_OUTPUT_LENGTH, "Removed breakpoint at %08x", breakpoint);
#endif
}

CONSOLE_COMMAND(Initializer::step_system) {
#ifdef DO_DEBUGGER
    if (argc < 2) {
        gba->paused = true;
        gba->stepcount = 1;
        STRCPY(output, MAX_OUTPUT_LENGTH, "Stepping system for one step");
    } else {
        gba->paused = true;
        u32 steps = parsedec(args[1]);
        gba->stepcount = steps;
        SPRINTF(output, MAX_OUTPUT_LENGTH, "Stepping system for %d steps", steps);
    }
#endif
}

u8 Initializer::ReadByte(u64 offset) {
    return gba->Memory.Read<u8, false>(offset);
}

u8* Initializer::ValidAddressMask(u32 address) {
    // all memory can be read for the GBA
    switch (static_cast<MemoryRegion>(address >> 24)) {
        case MemoryRegion::BIOS:
            return &gba->Memory.BIOS[address & 0x3fff];
        case MemoryRegion::Unused:
            return nullptr;
        case MemoryRegion::eWRAM:
            return &gba->Memory.eWRAM[address & 0x3'ffff];
        case MemoryRegion::iWRAM:
            return &gba->Memory.iWRAM[address & 0x7fff];
        case MemoryRegion::IO:
            return nullptr;
        case MemoryRegion::PAL:
            return &gba->Memory.PAL[address & 0x3ff];
        case MemoryRegion::VRAM:
            if ((address & 0x1'ffff) < 0x1'0000) {
                return &gba->Memory.VRAM[address & 0xffff];
            }
            return &gba->Memory.VRAM[0x1'0000 | (address & 0x7fff)];
        case MemoryRegion::OAM:
            return &gba->Memory.OAM[address & 0x3ff];
        case MemoryRegion::ROM_L1:
        case MemoryRegion::ROM_L2:
        case MemoryRegion::ROM_L:
        case MemoryRegion::ROM_H1:
        case MemoryRegion::ROM_H2:
        case MemoryRegion::ROM_H:
            return &gba->Memory.ROM[address & 0x01ff'ffff];
        case MemoryRegion::SRAM:
            return nullptr;
        default:
            return nullptr;
    }
}

bool Initializer::ARMMode() {
    return !(gba->CPU.CPSR & static_cast<u32>(CPSRFlags::T));
}

GBA* Initializer::init() {
    gba = (GBA *) malloc(sizeof(GBA));
    new(gba) GBA;

    frontend_init(
            &gba->shutdown,
            &gba->CPU.pc,
            0x1'0000'0000ULL,
            ValidAddressMask,
            ReadByte,
            nullptr,
            ARMMode
    );

    char label[0x100];

    int cpu_tab = add_register_tab("ARM7TDMI");

    for (int i = 0; i < 13; i++) {
        sprintf(label, "r%d", i);
        add_register_data(label, &gba->CPU.Registers[i], 4, cpu_tab);
    }

    add_register_data("r13 (SP)", &gba->CPU.sp, 4, cpu_tab);
    add_register_data("r14 (LR)", &gba->CPU.lr, 4, cpu_tab);
    add_register_data("r15 (PC)", &gba->CPU.pc, 4, cpu_tab);

    add_register_data("CPSR", &gba->CPU.CPSR, 4, cpu_tab);
    add_register_data("SPSR", &gba->CPU.SPSR, 4, cpu_tab);

    add_register_data("", NULL, 4, cpu_tab);
    add_register_data("", NULL, 4, cpu_tab);

    add_register_data("Pipe 0", &gba->CPU.Pipeline.Storage[0], 4, cpu_tab);
    add_register_data("Pipe 1", &gba->CPU.Pipeline.Storage[1], 4, cpu_tab);
    add_register_data("Pipe 2", &gba->CPU.Pipeline.Storage[2], 4, cpu_tab);
    add_register_data("Pipe 3", &gba->CPU.Pipeline.Storage[3], 4, cpu_tab);

    add_register_data("", NULL, 4, cpu_tab);
    add_register_data("", NULL, 8, cpu_tab);

    add_register_data("Time", &gba->CPU.timer, 8, cpu_tab);

    // add_command("RESET", "Resets the system. Add 'pause/freeze/break' to freeze on reload.", reset_system);
    add_command("PAUSE", "Pauses the system.", pause_system);
    add_command("CONTINUE", "Unpauses the system.", unpause_system);
    add_command("BREAK", "Add breakpoint to system at PC = $1.", break_system);
    add_command("UNBREAK", "Remove breakpoint to system at PC = $1.", unbreak_system);
    add_command("STEP", "Step system for $1 CPU steps (defaults to 1 step).", step_system);

    return gba;
}