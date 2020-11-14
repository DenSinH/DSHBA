#pragma once

#include "src/Frontend/interface.h"
#include "src/Core/system.h"

#include "default.h"

class Initializer {
public:
    static GBA* init();

private:
    static CONSOLE_COMMAND(step_system);
    static CONSOLE_COMMAND(trace_system);
    static CONSOLE_COMMAND(break_system);
    static CONSOLE_COMMAND(unbreak_system);
    static CONSOLE_COMMAND(pause_system);
    static CONSOLE_COMMAND(unpause_system);

    static MENU_ITEM_CALLBACK(toggle_frameskip);

    static u8 ReadByte(u64 offset);
    static u8* ValidAddressMask(u32 address);
    static bool ARMMode();
};
