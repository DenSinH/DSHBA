#include "src/Core/system.h"

#include "init.h"
#include "UIthread.h"
#include "sleeping.h"

#include "const.h"

static GBA* gba;

void exception_handler() {
    // keep open the screen to still be able to check the register/memory contents (unstable)
    while (!gba->shutdown) {
        sleep_ms(16);
    }
}

int main() {
    gba = Initializer::init();

#ifdef DO_BREAKPOINTS
    gba->paused = true;
#endif

    atexit(exception_handler);

    start_ui_thread();

    gba->Memory.LoadROM(ROM_FILE);
    gba->Run();

    join_ui_thread();

    return 0;
}
