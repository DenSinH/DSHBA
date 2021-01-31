#ifndef GC__LOG_H
#define GC__LOG_H

#include <stdio.h>
#include <stdlib.h>
#include "flags.h"


#ifdef _WIN32
    #include <Windows.h>

    #define FOREGROUND_YELLOW (FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN)
    #define FOREGROUND_PINK (FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE)
    #define FOREGROUND_WHITE (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)

    #define CONSOLE_BLUE() { SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_BLUE | FOREGROUND_INTENSITY); }
    #define CONSOLE_GREEN() { SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_INTENSITY); }
    #define CONSOLE_DARK_GREEN() { SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN); }
    #define CONSOLE_YELLOW() { SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_YELLOW); }
    #define CONSOLE_RED() { SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED); }
    #define CONSOLE_PINK() { SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_PINK); }
    #define CONSOLE_RESTORE() { SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_WHITE); }

#else

    // linux / macOS
    #define CONSOLE_BLUE() { fprintf(stdout, "\033[0;36m"); }
    #define CONSOLE_GREEN() { fprintf(stdout, "\033[0;32m"); }
    #define CONSOLE_DARK_GREEN() { fprintf(stdout, "\033[1;32m"); }
    #define CONSOLE_YELLOW() { fprintf(stdout, "\033[0;33m"); }
    #define CONSOLE_RED() { fprintf(stderr, "\033[1;31m"); }
    #define CONSOLE_PINK() { fprintf(stderr, "\033[0;35m"); }
    #define CONSOLE_RESTORE() { fprintf(stdout, "\033[0m"); }

#endif


#if COMPONENT_FLAGS & COMPONENT_CPU_VERBOSE
    #define log_cpu_verbose(message, ...) do {                        \
        CONSOLE_GREEN();                                \
        fprintf(stdout, "[CPU (verbose)]: " message "\n",  ##__VA_ARGS__); \
        CONSOLE_RESTORE();                             \
    } while(0)
#else
    #define log_cpu_verbose(message, ...) do { } while(0)
#endif

#if COMPONENT_FLAGS & (COMPONENT_CPU | COMPONENT_CPU_VERBOSE)
#define log_cpu(message, ...) do {                        \
        CONSOLE_DARK_GREEN();                                \
        fprintf(stdout, "[CPU]: " message "\n",  ##__VA_ARGS__); \
        CONSOLE_RESTORE();                             \
    } while(0)
#else
#define log_cpu(message, ...) do { } while(0)
#endif

#if COMPONENT_FLAGS & COMPONENT_PPU
#define log_ppu(message, ...) do {                        \
        CONSOLE_PINK();                                \
        fprintf(stdout, "[PPU]: " message "\n",  ##__VA_ARGS__); \
        CONSOLE_RESTORE();                             \
    } while(0)
#else
#define log_ppu(message, ...) do { } while(0)
#endif

#if COMPONENT_FLAGS & COMPONENT_IO
#define log_io(message, ...) do {                        \
        CONSOLE_DARK_GREEN();                                \
        fprintf(stdout, "[IO]: " message "\n",  ##__VA_ARGS__); \
        CONSOLE_RESTORE();                             \
    } while(0)
#else
#define log_io(message, ...) do { } while(0)
#endif

#if COMPONENT_FLAGS & COMPONENT_DMA
#define log_dma(message, ...) do {                        \
        CONSOLE_BLUE();                                \
        fprintf(stdout, "[DMA]: " message "\n",  ##__VA_ARGS__); \
        CONSOLE_RESTORE();                             \
    } while(0)
#else
#define log_dma(message, ...) do { } while(0)
#endif

#if COMPONENT_FLAGS & COMPONENT_TIMERS
#define log_tmr(message, ...) do {                        \
        CONSOLE_GREEN();                                \
        fprintf(stdout, "[TIMERS]: " message "\n",  ##__VA_ARGS__); \
        CONSOLE_RESTORE();                             \
    } while(0)
#else
#define log_tmr(message, ...) do { } while(0)
#endif

#if COMPONENT_FLAGS & COMPONENT_MEMORY
#define log_mem(message, ...) do {                        \
        CONSOLE_BLUE();                                \
        fprintf(stdout, "[MEMORY]: " message "\n",  ##__VA_ARGS__); \
        CONSOLE_RESTORE();                             \
    } while(0)
#else
#define log_mem(message, ...) do { } while(0)
#endif

#if COMPONENT_FLAGS & COMPONENT_SCHEDULER
#define log_sched(message, ...) do {                        \
        CONSOLE_BLUE();                                \
        fprintf(stdout, "[SCHEDULER]: " message "\n",  ##__VA_ARGS__); \
        CONSOLE_RESTORE();                             \
    } while(0)
#else
#define log_sched(message, ...) do { } while(0)
#endif

#if VERBOSITY <= VERBOSITY_ALL
#define log_any(message, ...) do {                        \
        CONSOLE_BLUE();                                \
        fprintf(stdout, message "\n",  ##__VA_ARGS__); \
        CONSOLE_RESTORE();                             \
    } while(0)
#else
#define log_any(message, ...) do { } while(0)
#endif

#if VERBOSITY <= VERBOSITY_DEBUG
    #define log_debug(message, ...) do {                 \
        fprintf(stdout, "[DEBUG]: ");                  \
        fprintf(stdout, message "\n", ##__VA_ARGS__); \
    } while(0)
#else
    #define log_debug(message, ...) do { } while(0)
#endif

#if VERBOSITY <= VERBOSITY_INFO
#define log_info(message, ...) do {                 \
        fprintf(stdout, "[INFO]: ");                  \
        fprintf(stdout, message "\n", ##__VA_ARGS__); \
    } while(0)
#else
#define log_info(message, ...) do { } while(0)
#endif

#if VERBOSITY <= VERBOSITY_WARN
    #define log_warn(message, ...) do {                  \
        CONSOLE_YELLOW();                             \
        fprintf(stdout, "[WARN]: ");                   \
        fprintf(stdout, message "\n", ##__VA_ARGS__); \
        CONSOLE_RESTORE();                            \
    } while(0)
#else
    #define log_warn(message, ...) do { } while(0)
#endif

#define log_fatal(message, ...) do {                                \
    CONSOLE_RED();                                               \
    fprintf(stderr, "[FATAL] at %s:%d: ", __FILE__, __LINE__);   \
    fprintf(stderr, message "\n", ##__VA_ARGS__);                \
    /* we're not restoring the color here for any handlers */    \
    exit(1);                                                     \
} while(0)

#endif //GC__LOG_H
