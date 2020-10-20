add_library(Core STATIC
        system.h
        system.cpp
        Breakpoints/breakpoints.h
        Breakpoints/breakpoints.c
        Scheduler/scheduler.c
        Scheduler/scheduler.h
        )

# add_subdirectory(HW)

# for logging / stuff I want to use in all files
include_directories(${PROJECT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include
#         $<TARGET_PROPERTY:HW,INCLUDE_DIRECTORIES>
        )

set_target_properties(Core PROPERTIES LINKER_LANGUAGE CXX)

# target_link_libraries(Core HW)

target_include_directories(Core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})