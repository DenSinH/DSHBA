find_package(SDL2 REQUIRED)

add_library(APU STATIC
        APU.cpp
        APU.h
        Channels/Channel.h Channels/Square.h Channels/Noise.h Channels/Wave.h Channels/EnvelopeChannel.h Channels/FIFO.h)

# for logging / stuff I want to use in all files
include_directories(
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/modules
        ../include
        # compatibility for linux
        ${SDL2_INCLUDE_DIR}
)

set_target_properties(APU PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(APU PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(APU PRIVATE ${SDL2_LIBRARY})