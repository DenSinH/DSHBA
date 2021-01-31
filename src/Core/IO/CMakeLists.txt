add_library(IO STATIC
        MMIO.h
        MMIO.cpp
        IORegisters.h
        IOFlags.h
        Interrupts.h
)

# for logging / stuff I want to use in all files
include_directories(
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/modules
        $<TARGET_PROPERTY:APU,INCLUDE_DIRECTORIES>  # need SDL stuff
        ../include
)

target_include_directories(IO PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})