#include <cstdio>

#include "src/Core/system.h"

#include "init.h"
#include "UIthread.h"


int main() {
    GBA* gba = init();
    gba->paused = true;

    start_ui_thread();

    gba->Run();

    join_ui_thread();

    return 0;
}
