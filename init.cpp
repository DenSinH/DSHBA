#include "init.h"

#include "default.h"
#include "debugging.h"

#include <memory>

static GBA* gba;

CONSOLE_COMMAND(Initializer::reset_system) {
#ifdef DO_DEBUGGER
    if (argc > 1 && (
            strcmp(args[1], "freeze") == 0 ||
            strcmp(args[1], "pause")  == 0 ||
            strcmp(args[1], "break")  == 0
    )
            ) {
        gba->Paused = true;
    }
#endif

    gba->Interaction = [] {
        gba->Reset();
        gba->ReloadROM();
    };
}

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

CONSOLE_COMMAND(Initializer::tester) {
#ifdef ADD_PPU
    gba->PPU.SyncToVideo ^= true;
    toggle_sync_to_video(gba->PPU.SyncToVideo);
#endif
}

static u64 ticks, prev_ticks;
OVERLAY_INFO(Initializer::cpu_ticks) {
    ticks = gba->timer;
    SPRINTF(output, output_length, "CPU ticks/s: %.1f", (float)(ticks - prev_ticks) / delta_time);
    prev_ticks = ticks;
}

static float accum_time;
static size_t prev_frame;
OVERLAY_INFO(Initializer::fps_counter) {
    accum_time += delta_time;
#ifdef ADD_PPU
    SPRINTF(
            output,
            output_length,
                "FPS        : %.1f\n"
                "Frame time : %.3fms",
            (double)(gba->PPU.Frame) / accum_time,
            1000.0 * delta_time / (float)(gba->PPU.Frame - prev_frame)
    );
    if (accum_time > 1) {
        gba->PPU.Frame = 0;
        accum_time = 0;
    }
    prev_frame = gba->PPU.Frame;
#else
    SPRINTF(output, output_length, "PPU not linked");
#endif
}

OVERLAY_INFO(Initializer::scheduler_events) {
    SPRINTF(
            output,
            output_length,
            "Scheduler  : %d events",
            gba->Scheduler.queue.size()
    );
}

OVERLAY_INFO(Initializer::scheduler_time_until) {
    SPRINTF(
            output,
            output_length,
            "Sched. next: %d cycles",
            gba->Scheduler.top - *gba->Scheduler.timer
    );
}

OVERLAY_INFO(Initializer::audio_samples) {
#ifdef ADD_APU
    if (gba->APU.Stream) {
        SPRINTF(
                output,
                output_length,
                "Samples    : %d bytes",
                SDL_AudioStreamAvailable(gba->APU.Stream)
        );
    }
    else {
        STRCPY(
                output,
                output_length,
                "NO STREAM AVAILABLE"
        );
    }
#else
    SPRINTF(output, output_length, "APU unlinked");
#endif
}

MENU_ITEM_CALLBACK(Initializer::toggle_frameskip) {
    // selected is PPU.FrameSkip
#ifdef ADD_PPU
    if (selected) {
        // just need to make sure we don't get stuck waiting for this
        gba->PPU.FrameDrawn = true;
        gba->PPU.FrameDrawnVariable.notify_all();
    }
#endif
}

MENU_ITEM_CALLBACK(Initializer::toggle_sync_to_video) {
    // selected is PPU.SyncToVideo
#ifdef ADD_PPU
    if (!selected) {
        // just need to make sure we don't get stuck waiting for this
        gba->PPU.FrameReady = true;
        gba->PPU.FrameReadyVariable.notify_all();
    }
#endif
}

static std::string file;
FILE_SELECT_CALLBACK(load_ROM_callback) {
    file = (std::string)file_name;
    gba->Interaction = []{ gba->LoadROM(file); };
}

MENU_ITEM_CALLBACK(load_ROM) {
    const char* filters[2] = {".gba", ""};
#ifdef ADD_FRONTEND
    open_file_explorer("Select ROM file", const_cast<char **>(filters), 2, load_ROM_callback);
#endif
}

u8 Initializer::ReadByte(u64 offset) {
    u8* ptr = gba->Memory.GetPtr(offset);
    if (ptr) {
        return *ptr;
    }
    return 0;
}

u8* Initializer::ValidAddressMask(u32 address) {
    // all memory can be read for the GBA
    return gba->Memory.GetPtr(address);
}

void Initializer::ParseInput(struct s_controller* controller) {
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

void Initializer::frontend_video_init() {
#ifdef ADD_PPU
    gba->PPU.VideoInit();
#endif
}

s_framebuffer Initializer::frontend_render(size_t t) {
#ifdef ADD_PPU
    return gba->PPU.RenderUntil(t);
#endif
    return {};
}

void Initializer::frontend_blit(const float* data) {
#ifdef ADD_PPU
    gba->PPU.Blit(data);
#endif
}

void Initializer::frontend_video_destroy() {
#ifdef ADD_PPU
    gba->PPU.VideoDestroy();
#endif
}

void Initializer::frontend_audio_init() {
#ifdef ADD_APU
    gba->APU.AudioInit();
#endif
}

void Initializer::frontend_audio_destroy() {
#ifdef ADD_APU
    gba->APU.AudioDestroy();
#endif
}

GBA* Initializer::init() {
    gba = new GBA;

#ifdef ADD_FRONTEND
    bind_video_init(frontend_video_init);
    bind_video_render(frontend_render);
    bind_video_blit(frontend_blit);
    bind_video_destroy(frontend_video_destroy);

    bind_audio_init(frontend_audio_init);
    bind_audio_destroy(frontend_audio_destroy);

    frontend_init(
            []{ gba->Shutdown = true; gba->Interaction = []{}; },  // need to set interaction function to break it out of the loop
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
    add_register_data("", nullptr, 4, cpu_tab);

    add_register_data("Time", &gba->timer, 4, cpu_tab);

    int IO_tab = add_register_tab("MMIO");

    add_register_data("DISPCNT", &gba->IO.DISPCNT, 2, IO_tab);
    add_register_data("DISPSTAT", &gba->IO.DISPSTAT, 2, IO_tab);
    add_register_data("VCOUNT", &gba->IO.VCount, 2, IO_tab);
    add_register_data("KEYINPUT", &gba->IO.KEYINPUT, 2, IO_tab);
    add_register_data("IME", &gba->CPU.IME, 2, IO_tab);
    add_register_data("IE", &gba->CPU.IE, 2, IO_tab);
    add_register_data("IF", &gba->CPU.IF, 2, IO_tab);

    add_register_data("", nullptr, 2, IO_tab);

    add_register_data("TM0CNT_L", &gba->IO.Timers[0].Register.CNT_L, 2, IO_tab);
    add_register_data("TM0CNT_H", &gba->IO.Timers[0].Register.CNT_H, 2, IO_tab);
    add_register_data("TM1CNT_L", &gba->IO.Timers[1].Register.CNT_L, 2, IO_tab);
    add_register_data("TM1CNT_H", &gba->IO.Timers[1].Register.CNT_H, 2, IO_tab);
    add_register_data("TM2CNT_L", &gba->IO.Timers[2].Register.CNT_L, 2, IO_tab);
    add_register_data("TM2CNT_H", &gba->IO.Timers[2].Register.CNT_H, 2, IO_tab);
    add_register_data("TM3CNT_L", &gba->IO.Timers[3].Register.CNT_L, 2, IO_tab);
    add_register_data("TM3CNT_H", &gba->IO.Timers[3].Register.CNT_H, 2, IO_tab);

#ifdef ADD_APU
    int APU_tab = add_register_tab("APU");

    add_register_data("SQ1Vol", &gba->APU.sq[0].Volume, 4, APU_tab);
    add_register_data("SQ1Per", &gba->APU.sq[0].Period, 4, APU_tab);
    add_register_data("SQ1Len", &gba->APU.sq[0].LengthCounter, 4, APU_tab);
    add_register_data("SQ1Smpl", &gba->APU.sq[0].CurrentSample, 2, APU_tab);
    add_register_data("", nullptr, 4, APU_tab);

    add_register_data("SQ2Vol", &gba->APU.sq[1].Volume, 4, APU_tab);
    add_register_data("SQ2Per", &gba->APU.sq[1].Period, 4, APU_tab);
    add_register_data("SQ2Len", &gba->APU.sq[1].LengthCounter, 4, APU_tab);
    add_register_data("SQ2Smpl", &gba->APU.sq[1].CurrentSample, 2, APU_tab);
    add_register_data("", nullptr, 4, APU_tab);

    add_register_data("NSVol", &gba->APU.ns.Volume, 4, APU_tab);
    add_register_data("NSPer", &gba->APU.ns.Period, 4, APU_tab);
    add_register_data("NSLen", &gba->APU.ns.LengthCounter, 4, APU_tab);
    add_register_data("NSSmpl", &gba->APU.ns.CurrentSample, 2, APU_tab);
    add_register_data("", nullptr, 4, APU_tab);

    add_register_data("WAVVol", &gba->APU.wav.Volume, 4, APU_tab);
    add_register_data("WAVPer", &gba->APU.wav.Period, 4, APU_tab);
    add_register_data("WAVLen", &gba->APU.wav.LengthCounter, 4, APU_tab);
    add_register_data("WAVPos", &gba->APU.wav.PositionCounter, 4, APU_tab);
    add_register_data("WAVSmpl", &gba->APU.wav.CurrentSample, 2, APU_tab);
    add_register_data("", nullptr, 4, APU_tab);

    add_register_data("SOUNDCNT_H", &gba->APU.SOUNDCNT_H, 2, APU_tab);
    add_register_data("SOUNDCNT_X", &gba->APU.SOUNDCNT_X, 2, APU_tab);
#endif

    int general_tab = add_register_tab("General");

    add_register_data("Top event", &gba->Scheduler.top, 8, general_tab);

    add_command("RESET", "Resets the system. Add 'pause/freeze/break' to freeze on reload.", reset_system);
    add_command("PAUSE", "Pauses the system.", pause_system);
    add_command("CONTINUE", "Unpauses the system.", unpause_system);
    add_command("BREAK", "Add breakpoint to system at PC = $1.", break_system);
    add_command("UNBREAK", "Remove breakpoint to system at PC = $1.", unbreak_system);
    add_command("STEP", "Step system for $1 CPU steps (defaults to 1 step).", step_system);
    add_command("TRACE", "Trace system for $1 CPU steps.", trace_system);
#ifndef NDEBUG
    add_command("TEST", "This is just for testing.", tester);
#endif

    add_overlay_info(cpu_ticks);
    add_overlay_info(fps_counter);
    add_overlay_info(scheduler_events);
    add_overlay_info(scheduler_time_until);
    add_overlay_info(audio_samples);

    int game_tab = add_menu_tab((char*)"Game");
    add_menu_item(game_tab, "Load ROM", nullptr, load_ROM);

#ifdef ADD_PPU
    int video_tab = add_menu_tab((char*)"Video");
    add_menu_item(video_tab, "Sync to video", &gba->PPU.SyncToVideo, Initializer::toggle_sync_to_video);
    add_menu_item(video_tab, "Frameskip", &gba->PPU.FrameSkip, Initializer::toggle_frameskip);

    add_menu_item(video_tab, "", nullptr, nullptr);

    add_menu_item(video_tab, "BG0", &gba->PPU.ExternalBGEnable[0], nullptr);
    add_menu_item(video_tab, "BG1", &gba->PPU.ExternalBGEnable[1], nullptr);
    add_menu_item(video_tab, "BG2", &gba->PPU.ExternalBGEnable[2], nullptr);
    add_menu_item(video_tab, "BG3", &gba->PPU.ExternalBGEnable[3], nullptr);
    add_menu_item(video_tab, "OBJ", &gba->PPU.ExternalObjEnable, nullptr);
#endif

#ifdef ADD_APU
    int audio_tab = add_menu_tab((char*)"Audio");
    const float max_volume = 2.0;

    add_menu_item(audio_tab, "Master Enable", &gba->APU.ExternalEnable, nullptr);
    add_sliderf(audio_tab, "Master Volume", &gba->APU.ExternalVolume, 0.0, max_volume);

    int fifo_a_menu = add_submenu(audio_tab, "FIFO A");
    add_submenu_item(audio_tab, fifo_a_menu, "Enable", &gba->APU.ExternalFIFOEnable[0], nullptr);
    add_submenu_sliderf(audio_tab, fifo_a_menu, "Volume", &gba->APU.ExternalFIFOVolume[0], 0.0, max_volume);

    int fifo_b_menu = add_submenu(audio_tab, "FIFO B");
    add_submenu_item(audio_tab, fifo_b_menu, "Enable", &gba->APU.ExternalFIFOEnable[1], nullptr);
    add_submenu_sliderf(audio_tab, fifo_b_menu, "Volume", &gba->APU.ExternalFIFOVolume[1], 0.0, max_volume);

    // separator
    add_menu_item(audio_tab, "", nullptr, nullptr);

    const char* channel_names[4] = {
            "Square 0",
            "Square 1",
            "Noise",
            "Wave"
    };

    for (int i = 0; i < 4; i++) {
        int channel_menu = add_submenu(audio_tab, channel_names[i]);
        add_submenu_item(audio_tab, channel_menu, "Enable", &gba->APU.ExternalChannelEnable[i], nullptr);
        add_submenu_sliderf(audio_tab, channel_menu, "Volume", &gba->APU.ExternalChannelVolume[i], 0.0, max_volume);
    }
#endif  // ADD_APU
#endif
    return gba;
}