add_library(Core STATIC
        system.h
        system.cpp
        Breakpoints/breakpoints.h
        Breakpoints/breakpoints.c
        Scheduler/scheduler.c
        Scheduler/scheduler.h
        CoreUtils.cpp include/CoreUtils.h)

add_subdirectory(Mem)
add_subdirectory(ARM7TDMI)

# for logging / stuff I want to use in all files
include_directories(
        ${PROJECT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        $<TARGET_PROPERTY:Mem,INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:ARM7TDMI,INCLUDE_DIRECTORIES>
)

set_target_properties(Core PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(Core Mem)
target_link_libraries(Core ARM7TDMI)

target_include_directories(Core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})