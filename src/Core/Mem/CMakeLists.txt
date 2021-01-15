add_library(Mem STATIC
        Mem.h
        Mem.cpp
        MemoryHelpers.h
        Backup/BackupMem.h
        Backup/SRAM.cpp
        Backup/SRAM.h
        Backup/Flash.cpp
        Backup/Flash.h
        Backup/BackupDB.h
        Backup/BackupDefault.h
        Backup/EEPROM.cpp
        Backup/EEPROM.h
        NormmattBIOS.h MemPageTables.inl)

# for logging / stuff I want to use in all files
include_directories(
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/modules
        ../include
)

if (ADD_APU)
    include_directories(
            $<TARGET_PROPERTY:APU,INCLUDE_DIRECTORIES>  # need SDL stuff
    )
endif()

set_target_properties(Mem PROPERTIES LINKER_LANGUAGE CXX)


target_include_directories(Mem PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})