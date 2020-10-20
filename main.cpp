#include "src/Core/system.h"

#include "init.h"
#include "UIthread.h"


int main() {
    printf("Hello World!\n");
    GBA* gba = init();
    gba->paused = true;

    start_ui_thread();

    gba->Run();

    join_ui_thread();

    return 0;
}
