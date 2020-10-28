#include "src/Core/system.h"
#include "src/Frontend/interface.h"

#include "init.h"
#include "sleeping.h"

#include "const.h"

#include <thread>

static GBA* gba;

void exception_handler() {
    // keep open the screen to still be able to check the register/memory contents (unstable)
    while (!gba->Shutdown) {
        sleep_ms(16);
    }
}

#undef BENCHMARKING
#ifdef BENCHMARKING

#include <chrono>
const size_t tests = 500000;

void benchmark() {
    auto start = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < tests; i++) {
        gba->CPU.Registers[i & 0xf] = gba->CPU.sbcs_cv(i, i + 1, i & 1);
    }

    auto elapsed = std::chrono::high_resolution_clock::now() - start;

    printf("First one: %lldus\n", std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count());

    start = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < tests; i++) {
        gba->CPU.Registers[i & 0xf] = gba->CPU.sbcs_cv_old(i, i + 1, i & 1);
    }

    elapsed = std::chrono::high_resolution_clock::now() - start;

    printf("Second one: %lldus\n", std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count());
}
#endif

int main() {
    gba = Initializer::init();

#ifdef BENCHMARKING
    benchmark();
    return 0;
#endif

#ifdef DO_BREAKPOINTS
    gba->Paused = false;
#endif

    atexit(exception_handler);

    std::thread ui_thread(ui_run);

    gba->Memory.LoadROM("D:/Data/CProjects/DSHBA/files/roms/thumb.gba");
    gba->Run();

    ui_thread.join();

    return 0;
}
