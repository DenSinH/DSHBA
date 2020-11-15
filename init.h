#pragma once

#include "src/Frontend/interface.h"
#include "src/Core/system.h"

#include "default.h"

class Initializer {
public:
    static GBA* init();

private:
    static CONSOLE_COMMAND(reset_system);
    static CONSOLE_COMMAND(step_system);
    static CONSOLE_COMMAND(trace_system);
    static CONSOLE_COMMAND(break_system);
    static CONSOLE_COMMAND(unbreak_system);
    static CONSOLE_COMMAND(pause_system);
    static CONSOLE_COMMAND(unpause_system);
    static CONSOLE_COMMAND(tester);

    static MENU_ITEM_CALLBACK(toggle_frameskip);
    static MENU_ITEM_CALLBACK(toggle_sync_to_video);

    static OVERLAY_INFO(cpu_ticks);
    static OVERLAY_INFO(fps_counter);

    static u8 ReadByte(u64 offset);
    static u8* ValidAddressMask(u32 address);
    static void ParseInput(struct s_controller* controller);

    static void frontend_video_init();
    static s_framebuffer frontend_render(size_t);
    static void frontend_destroy();

    static bool ARMMode();
};
