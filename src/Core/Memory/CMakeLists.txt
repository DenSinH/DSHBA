add_library(Memory STATIC
        Memory.h
        Memory.cpp
        MemoryHelpers.h)

set(CMAKE_CXX_STANDARD 17)

# for logging / stuff I want to use in all files
include_directories(
        ${PROJECT_SOURCE_DIR}/include
#         ${CMAKE_CURRENT_SOURCE_DIR}/include
        ../include
)

set_target_properties(Memory PROPERTIES LINKER_LANGUAGE CXX)


target_include_directories(Memory PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})