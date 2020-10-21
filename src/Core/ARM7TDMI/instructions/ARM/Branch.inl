/*
 * INL files will be "inline" included in .cpp files
 * This allows us to do templated class functions easier
 * */

#ifndef INLINED_INCLUDES
#include "../../ARM7TDMI.h"

class ARM7TDMI_INL : ARM7TDMI {
#endif

template<bool Link>
void Branch(u32 instruction) {
    if constexpr(Link) {
        this->lr = this->pc - 4;
    }

    i32 offset = (i32)(instruction << 8) >> 6;  // shifted left by 2 and sign extended

    this->pc += offset;
    this->FlushPipeline();
}

#ifndef INLINED_INCLUDES
};
#endif