cmake_minimum_required(VERSION 3.16)
project(DSHBA LANGUAGES C CXX)

# set(CMAKE_VERBOSE_MAKEFILE ON)

set(ADD_PPU 0)
set(ADD_FRONTEND 0)
set(ADD_APU 0)

# check of LTO
include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

set(CMAKE_CXX_FLAGS_RELEASE "-Ofast") # -march=native
set(CMAKE_C_FLAGS_RELEASE "-Ofast")
if (ADD_PPU)
    add_compile_definitions(ADD_PPU=${ADD_PPU})
endif()
if (ADD_APU)
    add_compile_definitions(ADD_APU=${ADD_APU})
endif()
if (ADD_FRONTEND)
    add_compile_definitions(ADD_FRONTEND=${ADD_FRONTEND})
endif()

if (NOT WIN32)
    # we need to specify this
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif()

if (CMAKE_COMPILER_IS_GNUCC)
    message(STATUS "GCC compiler detected")
    # for some reason, GCC complained about this
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mbmi")
endif()

# add_compile_options($<$<CONFIG:DEBUG>:-Wall>)
add_compile_definitions(
        $<$<CONFIG:RELEASE>:NDEBUG>
        $<$<CONFIG:RELEASE>:BOOST_DISABLE_ASSERTS>
)

add_executable(DSHBA
        main.cpp
        include/default.h
        include/helpers.h
        include/flags.h
        include/log.h
        include/sleeping.h
        include/const.h
        modules/glad/glad.h
        modules/glad/glad.c
        modules/KHR/khrplatform.h
        debugging.h
        init.cpp
        init.h
)

# add link time optimizations for extra speed
if( ipo_supported )
    message(STATUS "${CMAKE_BUILD_TYPE}")
    if (CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo" OR CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
        message(STATUS "IPO / LTO enabled")
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    else()
        message(STATUS "IPO / LTO enabled when compiled in release mode")
    endif ()
else()
    message(STATUS "IPO / LTO not supported: <${ipo_error}>")
endif()

add_subdirectory(src/Core)

if (ADD_FRONTEND)
    add_subdirectory(modules/imgui)
    add_subdirectory(src/Frontend)
endif()

include_directories(
        include
        modules
        $<TARGET_PROPERTY:Core,INCLUDE_DIRECTORIES>
)

if (ADD_FRONTEND)
    include_directories($<TARGET_PROPERTY:Frontend,INCLUDE_DIRECTORIES>)

    if (WIN32)
        target_link_libraries(DSHBA PRIVATE Core Frontend imgui)
    else()
        target_link_libraries(DSHBA PRIVATE Core Frontend imgui ${CMAKE_DL_LIBS})
    endif()
else()
    if (WIN32)
        target_link_libraries(DSHBA PRIVATE Core)
    else()
        target_link_libraries(DSHBA PRIVATE Core ${CMAKE_DL_LIBS})
    endif()
endif()

# copy over files with settings
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/imgui.ini
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/input.map
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR})