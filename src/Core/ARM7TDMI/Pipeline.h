#ifndef DSHBA_PIPELINE_H
#define DSHBA_PIPELINE_H

#include "default.h"


typedef struct {
private:
    friend class Initializer;
    u32 Storage[4] = {};
    u32 Offset = 0;

public:
    u32 Count = 0;

    void Clear() {
        Offset = Count = 0;
    }

    u32 Peek() {
        return Storage[Offset & 3];
    }

    u32 Dequeue() {
        Count--;
        return Storage[Offset++ & 3];
    }

    void Enqueue(u32 value) {
        Storage[(Offset + Count++) & 3] = value;
    }
} s_Pipeline;

#endif //DSHBA_PIPELINE_H
