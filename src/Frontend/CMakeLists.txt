find_package(SDL2 REQUIRED)

set(FOUND_CAPSTONE 0)
if (WIN32)
    find_library(Capstone capstone_dll)
    if(Capstone)
        message(STATUS "Found Capstone")
        add_definitions(-DDO_CAPSTONE)
        set(FOUND_CAPSTONE 1)
    else()
        message(WARNING "Could not find Capstone, disassembly disabled")
    endif()
else()
    find_package(Capstone)
    if (Capstone_FOUND)
        message(STATUS "Found Capstone")
        add_definitions(-DDO_CAPSTONE)
        set(FOUND_CAPSTONE 1)
    else()
        message(WARNING "Could not find Capstone, disassembly disabled")
    endif()
endif()

if (FOUND_CAPSTONE)
    find_path(Capstone_INCLUDE_DIR capstone.h PATH_SUFFIXES capstone)
    set(Capstone_INCLUDE_DIR ${Capstone_INCLUDE_DIR}/../)
endif()

set(FRONTEND_SRCS
        imgui/imgui_impl_opengl3.cpp
        imgui/imgui_impl_sdl.cpp
        frontend.cpp
        widgets/menubar.cpp
        widgets/disassemble.c
        controller.cpp
        debugger.cpp
        settings.cpp
)

set(FRONTEND_HDRS
        imgui/imgui_impl_opengl3.h
        imgui/imgui_impl_sdl.h
        widgets/memory_viewer.h
        widgets/console.h
        widgets/menubar.h
        widgets/register_viewer.h
        widgets/disassembly_viewer.h
        widgets/overlay.h
        widgets/disassemble.h
        widgets/file_dialog.h
        debugger.h
        frontend.h
        interface.h
        controller.h
        settings.h
)

add_library(Frontend ${FRONTEND_SRCS} ${FRONTEND_HDRS})

set_target_properties(Frontend PROPERTIES
                      LINKER_LANGUAGE CXX
                      CXX_STANDARD 20
                      CXX_STANDARD_REQUIRED YES
                      CXX_EXTENSIONS ON)

target_link_libraries(Frontend PRIVATE ${SDL2_LIBRARY})

if (FOUND_CAPSTONE)
    if (WIN32)
        target_link_libraries(Frontend PRIVATE ${Capstone})
    else()
        target_link_libraries(Frontend PRIVATE Capstone::Capstone)
    endif()
endif()

if (NOT WIN32)
    target_link_libraries(Frontend PRIVATE -ldl -lpthread)
endif()

target_include_directories(Frontend PRIVATE
        imgui
        ../../include
        ../../modules
        ${SDL2_INCLUDE_DIR}
        ${Capstone_INCLUDE_DIR}
)