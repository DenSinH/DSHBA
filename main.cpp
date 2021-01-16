#include "init.h"
#include "sleeping.h"

#include "const.h"

#include <thread>

static GBA* gba;

void exception_handler() {
#ifdef ADD_FRONTEND
    // keep open the screen to still be able to check the register/memory contents (unstable)
    while (!gba->Shutdown) {
        sleep_ms(16);
    }
#endif
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

#undef main
int main() {
    gba = Initializer::init();

    // gba->LoadBIOS(std::string("D:\\Data\\GBA\\BIOS\\bios.bin"));
    gba->LoadBIOS(std::string(BIOS_FILE));
    printf("GBA system size (bytes): %x", sizeof(GBA));

#ifdef BENCHMARKING
    benchmark();
    return 0;
#endif

#ifdef DO_BREAKPOINTS
    gba->Paused = false;
#endif

#ifdef ADD_FRONTEND
    atexit(exception_handler);

    std::thread ui_thread(ui_run);
#endif

    gba->LoadROM("D:\\User\\Downloads\\Pokemon - Emerald Version (USA, Europe).gba");
    gba->Run();

#ifdef ADD_FRONTEND
    ui_thread.join();
#endif

    return 0;
}
