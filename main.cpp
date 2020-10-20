#include "src/Core/system.h"

#include "init.h"
#include "UIthread.h"

#include "const.h"

int main() {
    GBA* gba = init();
    gba->paused = true;

    start_ui_thread();

    gba->memory.LoadROM(ROM_FILE);
    gba->Run();

    join_ui_thread();

    return 0;
}
