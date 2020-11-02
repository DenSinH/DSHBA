add_library(Mem STATIC
        Mem.h
        Mem.cpp
        MemoryHelpers.h)

# for logging / stuff I want to use in all files
include_directories(
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/modules
        ../include
)

set_target_properties(Mem PROPERTIES LINKER_LANGUAGE CXX)


target_include_directories(Mem PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})