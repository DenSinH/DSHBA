#include <cstdio>

#include "system.h"
#include "sleeping.h"
#include "const.h"

#include "flags.h"

GBA::GBA() {

}

GBA::~GBA() {

}

void GBA::Reset() {
    CPU.Reset();
    Memory.Reset();
    IO.Reset();
}

void GBA::Run() {
    CPU.SkipBIOS();

    while (!Shutdown) {

        while (!Interaction) {
           CPU.Run(reinterpret_cast<void **>(&Interaction));
        }

        Interaction();
        Interaction = nullptr;
    }
}