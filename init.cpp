#include "init.h"

#include "default.h"
#include "debugging.h"

#include <memory>

static GBA* gba;

CONSOLE_COMMAND(Initializer::pause_system) {
#ifdef DO_DEBUGGER
    gba->Paused = true;

    STRCPY(output, MAX_OUTPUT_LENGTH, "System paused");
#endif
}

CONSOLE_COMMAND(Initializer::unpause_system) {
#ifdef DO_DEBUGGER
    gba->Paused = false;

    STRCPY(output, MAX_OUTPUT_LENGTH, "System unpaused");
#endif
}

CONSOLE_COMMAND(Initializer::break_system) {
#ifdef DO_DEBUGGER
    if (argc < 2) {
        pause_system(args, argc, output);
        return;
    }

#ifdef DO_BREAKPOINTS
    u32 breakpoint = parsehex(args[1]);
    add_breakpoint(&gba->Breakpoints, breakpoint);
    SPRINTF(output, MAX_OUTPUT_LENGTH, "Added breakpoint at %08x", breakpoint);
#else
    STRCPY(output, MAX_OUTPUT_LENGTH, "Breakpoints disabled");
#endif
#endif
}

CONSOLE_COMMAND(Initializer::unbreak_system) {
#ifdef DO_DEBUGGER
    if (argc < 2) {
        unpause_system(args, argc, output);
        return;
    }
#ifdef DO_BREAKPOINTS
    u32 breakpoint = parsehex(args[1]);
    remove_breakpoint(&gba->Breakpoints, breakpoint);
    SPRINTF(output, MAX_OUTPUT_LENGTH, "Removed breakpoint at %08x", breakpoint);
#else
    STRCPY(output, MAX_OUTPUT_LENGTH, "Breakpoints disabled");
#endif
#endif
}

CONSOLE_COMMAND(Initializer::step_system) {
#ifdef DO_DEBUGGER
    if (argc < 2) {
        gba->Paused = true;
        gba->Stepcount = 1;
        STRCPY(output, MAX_OUTPUT_LENGTH, "Stepping system for one step");
    } else {
        gba->Paused = true;
        u32 steps = parsedec(args[1]);
        gba->Stepcount = steps;
        SPRINTF(output, MAX_OUTPUT_LENGTH, "Stepping system for %d steps", steps);
    }
#endif
}

CONSOLE_COMMAND(Initializer::trace_system) {
#ifdef DO_DEBUGGER
    if (argc < 2) {
        STRCPY(output, MAX_OUTPUT_LENGTH, "Please give a number of steps");
    } else {
#ifdef TRACE_LOG
        u32 steps = parsedec(args[1]);
        gba->CPU.TraceSteps = steps;
        SPRINTF(output, MAX_OUTPUT_LENGTH, "Tracing system for %d steps", steps);
#else
        STRCPY(output, MAX_OUTPUT_LENGTH, "Trace logging is turned off");
#endif
    }
#endif
}

static u64 ticks, prev_ticks;
static OVERLAY_INFO(cpu_ticks) {
    ticks = gba->CPU.timer;
    SPRINTF(output, output_length, "CPU ticks/s: %.1f", (float)(ticks - prev_ticks) / delta_time);
    prev_ticks = ticks;
}

static float accum_time;
static OVERLAY_INFO(fps_counter) {
    accum_time += delta_time;
    SPRINTF(output, output_length, "FPS        : %.1f", (double)(gba->PPU.Frame) / accum_time);
    if (accum_time > 1) {
        gba->PPU.Frame = 0;
        accum_time = 0;
    }
}

u8 Initializer::ReadByte(u64 offset) {
    return gba->Memory.Read<u8, false>(offset);
}

u8* Initializer::ValidAddressMask(u32 address) {
    // all memory can be read for the GBA
    return gba->Memory.GetPtr(address);
}

static void ParseInput(struct s_controller* controller) {
    u16 KEYINPUT = 0;
    if (controller->A | controller->X) KEYINPUT |= static_cast<u16>(KeypadButton::A);
    if (controller->B | controller->Y) KEYINPUT |= static_cast<u16>(KeypadButton::B);
    if (controller->start)             KEYINPUT |= static_cast<u16>(KeypadButton::Start);
    if (controller->select)            KEYINPUT |= static_cast<u16>(KeypadButton::Select);
    if (controller->left)              KEYINPUT |= static_cast<u16>(KeypadButton::Left);
    if (controller->right)             KEYINPUT |= static_cast<u16>(KeypadButton::Right);
    if (controller->down)              KEYINPUT |= static_cast<u16>(KeypadButton::Down);
    if (controller->up)                KEYINPUT |= static_cast<u16>(KeypadButton::Up);
    if (controller->L)                 KEYINPUT |= static_cast<u16>(KeypadButton::L);
    if (controller->R)                 KEYINPUT |= static_cast<u16>(KeypadButton::R);

    // bits are flipped
    gba->IO.KEYINPUT = ~KEYINPUT;
}

bool Initializer::ARMMode() {
    return !(gba->CPU.CPSR & static_cast<u32>(CPSRFlags::T));
}

static void frontend_video_init() {
    gba->PPU.VideoInit();
}

static s_framebuffer frontend_render() {
    return gba->PPU.Render();
}

GBA* Initializer::init() {
    gba = (GBA *) malloc(sizeof(GBA));
    new(gba) GBA;

    bind_video_init(frontend_video_init);
    bind_video_render(frontend_render);

    frontend_init(
            &gba->Shutdown,
            &gba->CPU.pc,
            0x1'0000'0000ULL,
            ValidAddressMask,
            ReadByte,
            ParseInput,
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

    add_register_data("", nullptr, 4, cpu_tab);
    add_register_data("", nullptr, 4, cpu_tab);

    add_register_data("Pipe 0", &gba->CPU.Pipeline.Storage[0], 4, cpu_tab);
    add_register_data("Pipe 1", &gba->CPU.Pipeline.Storage[1], 4, cpu_tab);
    add_register_data("Pipe 2", &gba->CPU.Pipeline.Storage[2], 4, cpu_tab);
    add_register_data("Pipe 3", &gba->CPU.Pipeline.Storage[3], 4, cpu_tab);

    add_register_data("", nullptr, 4, cpu_tab);
    add_register_data("", nullptr, 8, cpu_tab);

    add_register_data("Time", &gba->CPU.timer, 8, cpu_tab);

    int IO_tab = add_register_tab("MMIO");

    add_register_data("DISPCNT", &gba->IO.Registers[0], 2, IO_tab);
    add_register_data("DISPSTAT", &gba->IO.DISPSTAT, 2, IO_tab);
    add_register_data("VCOUNT", &gba->IO.VCount, 2, IO_tab);
    add_register_data("KEYINPUT", &gba->IO.KEYINPUT, 2, IO_tab);
    add_register_data("IME", &gba->CPU.IME, 2, IO_tab);
    add_register_data("IE", &gba->CPU.IE, 2, IO_tab);
    add_register_data("IF", &gba->CPU.IF, 2, IO_tab);

    // add_command("RESET", "Resets the system. Add 'pause/freeze/break' to freeze on reload.", reset_system);
    add_command("PAUSE", "Pauses the system.", pause_system);
    add_command("CONTINUE", "Unpauses the system.", unpause_system);
    add_command("BREAK", "Add breakpoint to system at PC = $1.", break_system);
    add_command("UNBREAK", "Remove breakpoint to system at PC = $1.", unbreak_system);
    add_command("STEP", "Step system for $1 CPU steps (defaults to 1 step).", step_system);
    add_command("TRACE", "Trace system for $1 CPU steps.", trace_system);

    add_overlay_info(cpu_ticks);
    add_overlay_info(fps_counter);

    return gba;
}