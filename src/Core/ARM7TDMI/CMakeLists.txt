add_library(ARM7TDMI STATIC
        ARM7TDMI.h
        ARM7TDMI.cpp
        Pipeline.h
)

# for logging / stuff I want to use in all files
include_directories(
        ${PROJECT_SOURCE_DIR}/include
        #         ${CMAKE_CURRENT_SOURCE_DIR}/include
        ../include
)

set_target_properties(ARM7TDMI PROPERTIES LINKER_LANGUAGE CXX)


target_include_directories(ARM7TDMI PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})