#ifndef GC__FLAGS_H
#define GC__FLAGS_H

#define VERBOSITY_ALL 0
#define VERBOSITY_DEBUG 1
#define VERBOSITY_INFO 2
#define VERBOSITY_WARN 3
#define VERBOSITY_ERROR 4
#define VERBOSITY_MAX VERBOSITY_ERROR

#define COMPONENT_CPU_VERBOSE 0x01
#define COMPONENT_SCHEDULER   0x02
#define COMPONENT_CPU         0x04
#define COMPONENT_PPU         0x08
#define COMPONENT_IO          0x10
#define COMPONENT_DMA         0x20
#define COMPONENT_TIMERS      0x40
#define COMPONENT_MEMORY      0x80

#define STUB_SIO

#undef SINGLE_CPI

#ifndef NDEBUG
// change to change verbosity / component logging:
#define VERBOSITY VERBOSITY_DEBUG
#define COMPONENT_FLAGS (0)

// very intense testing variables:
#define TRACE_LOG
#undef ALWAYS_TRACE_LOG
#undef FULL_VRAM_BUFFER

// checks
#define DO_DEBUGGER
#define DO_BREAKPOINTS
#define CHECK_INVALID_REFLUSHES
#define CHECK_SCANLINE_BATCH_ACCUM

// optimizations
#define DIRTY_MEMORY_ACCESS
#define FAST_ADD_SUB
#define BASIC_IDLE_DETECTION
#define CTTZ_LDM_STM_LOOP_BASE  // not sure if this is worth it (loop unrolling might be faster)
#define DIRECT_IO_DATA_COPY
#define FAST_DMA

#else
#define VERBOSITY VERBOSITY_MAX

#define DIRTY_MEMORY_ACCESS      // pointer memory access (only works on little endian)
#define FAST_ADD_SUB             // add/sub/adc/subc based on instrinsics/builtins
#define BASIC_IDLE_DETECTION     // detect idle branches in ARM/THUMB branch (without link) instructions
#define CTTZ_LDM_STM_LOOP_BASE   // in LDM/STM type instructions, use cttz(rlist) as lower bound for the loop instead of 0
#define DIRECT_IO_DATA_COPY      // use memcpy to copy the register values into DMA data structs (only works on little endian)
#define FAST_DMA                 // use memcpy/pointer copies to do DMA transfers

#endif

#endif //GC__FLAGS_H
