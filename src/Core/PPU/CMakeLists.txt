find_package(SDL2 CONFIG REQUIRED)

# compatibility for linux
include_directories(${SDL2_INCLUDE_DIRS})

file(GLOB GLSL_FILES ${CMAKE_CURRENT_SOURCE_DIR}/shaders/raw/*.glsl)

message(STATUS "Found GLSL shaders: ${GLSL_FILES}")

if (WIN32)
    add_custom_command(
            OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/shaders/shaders.h
            COMMENT "Generating shaders/shaders.h"
            COMMAND py ${CMAKE_CURRENT_SOURCE_DIR}/shaders/shaders.py
            DEPENDS ${GLSL_FILES}
    )
else()
    # linux/mac use python in command line instead of py
    add_custom_command(
            OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/shaders/shaders.h
            COMMENT "Generating shaders/shaders.h"
            COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/shaders/shaders.py
            DEPENDS ${GLSL_FILES}
    )
endif()

set_source_files_properties(
        shaders/shaders.h PROPERTIES
        GENERATED TRUE
)

add_library(PPU STATIC
        PPU.cpp
        PPU.h
        shaders/shaders.h
        shaders/GX_constants.h
        GXHelpers.h)

# for logging / stuff I want to use in all files
include_directories(
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/modules
        #         ${CMAKE_CURRENT_SOURCE_DIR}/include
        ../include
)

set_target_properties(PPU PROPERTIES LINKER_LANGUAGE CXX)
if (WIN32)
    target_link_libraries(PPU PRIVATE SDL2::SDL2 SDL2::SDL2main)
else()
    target_link_libraries(PPU PRIVATE ${SDL2_LIBRARIES})
endif()

target_include_directories(PPU PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})