add_library(Core STATIC
        system.h
        system.cpp
        Breakpoints/breakpoints.h
        Breakpoints/breakpoints.c
        Scheduler/scheduler.h
        CoreUtils.cpp
        include/CoreUtils.h
        )

add_subdirectory(Mem)
add_subdirectory(ARM7TDMI)
if (ADD_PPU)
    add_subdirectory(PPU)
endif()
add_subdirectory(APU)
add_subdirectory(IO)

include_directories(
        ${PROJECT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        $<TARGET_PROPERTY:Mem,INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:ARM7TDMI,INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:APU,INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:IO,INCLUDE_DIRECTORIES>
)

if (ADD_PPU)
    include_directories($<TARGET_PROPERTY:PPU,INCLUDE_DIRECTORIES>)
endif()

set_target_properties(Core PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(Core Mem)
target_link_libraries(Core ARM7TDMI)
if (ADD_PPU)
    target_link_libraries(Core PPU)
endif()
target_link_libraries(Core APU)
target_link_libraries(Core IO)

target_include_directories(Core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})