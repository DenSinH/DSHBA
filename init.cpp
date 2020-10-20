#include "init.h"
#include "src/Frontend/interface.h"

#include "default.h"

#include <memory>

static GBA* gba;

GBA* init() {
    gba = (GBA*)malloc(sizeof(GBA));
    new(gba) GBA;

    frontend_init(
            &gba->shutdown,
            nullptr,
            nullptr,
            0,
            nullptr,
            nullptr,
            nullptr
    );

    return gba;
}
