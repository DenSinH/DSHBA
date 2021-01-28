#include "init.h"
#include "sleeping.h"

#include "const.h"

#include <thread>

static GBA* gba;

void exception_handler() {
    // keep open the screen to still be able to check the register/memory contents (unstable)
    gba->CPU.Paused = true;
    gba->PPU.SyncToVideo = true;
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

#undef main
int main() {
    gba = Initializer::init();

    // gba->LoadBIOS(std::string("D:\\Data\\GBA\\BIOS\\bios.bin"));
    gba->LoadBIOS(std::string(BIOS_FILE));

#ifdef BENCHMARKING
    benchmark();
    return 0;
#endif

#ifdef DO_BREAKPOINTS
    gba->CPU.Paused = false;
#endif

    atexit(exception_handler);

    std::thread ui_thread(ui_run);

    gba->PPU.SyncToVideo = true;
    gba->APU.ExternalEnable = false;
    // gba->LoadROM("D:\\Data\\GBA\\Dash\\dash.gba");
    // gba->LoadROM("D:\\User\\Downloads\\Doom (USA, Europe).gba");
    // gba->LoadROM("D:\\User\\Downloads\\Legend of Zelda, The - The Minish Cap (USA).gba");
    gba->LoadROM("D:\\User\\Downloads\\AGS.gba");
    // gba->LoadROM("D:\\User\\Downloads\\suite.gba");
    // gba->LoadROM("D:\\Data\\CProjects\\DSHBA\\files\\roms\\main.gba");
    // gba->LoadROM("D:\\Data\\CProjects\\DSHBA\\files\\tonc\\win_demo.gba");
    // gba->LoadROM("D:\\Data\\CProjects\\DSHBA\\files\\krom\\BIOS\\Arithmetic\\ARCTAN\\BIOSARCTAN.gba");
    gba->Run();

    ui_thread.join();

    return 0;
}
