add_library(IO STATIC
        MMIO.h
        MMIO.cpp
        Registers.h
        IOFlags.h
        Interrupts.h)

# for logging / stuff I want to use in all files
include_directories(
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/modules
        ../include
)

target_include_directories(IO PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})