cmake_minimum_required(VERSION 3.18.0)


# list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
# list(APPEND CMAKE_FIND_ROOT_PATH ${CMAKE_SOURCE_DIR}/cmake)


project(TotalGB
    VERSION 0.0.1
    DESCRIPTION "Simple, fast, very inaccurate GB / GBC emulator!"
    HOMEPAGE_URL "https://github.com/ITotalJustice/TotalGB"
    LANGUAGES C
)


message("SYSTEM: ${CMAKE_SYSTEM_NAME}")
message("COMPILER: ${CMAKE_C_COMPILER_ID}")

# SEE: https://cmake.org/cmake/help/latest/manual/cmake-compile-features.7.html#supported-compilers
list(APPEND cmake_supported_compiler_features
    AppleClang
    Clang
    GNU
    MSVC
    SunPro
    Intel
)

if (${CMAKE_C_COMPILER_ID} IN_LIST cmake_supported_compiler_features)
    set(COMPILER_FEATURES ON)
    message("compiler features are enabled for this compiler")
else()
    set(COMPILER_FEATURES OFF)
    message("compiler features are NOT enabled for this compiler")
endif()


option(GB_SINGLE_FILE "include all src in single.c" OFF)
option(GB_DEBUG "enable debug" OFF)
option(GB_DEV "enables debug and sanitizers" OFF)
option(GB_ENABLE_FORCE_INLINE "enables force inline, disabled in debug mode" ON)

option(GBC_ENABLE "build with GBC support" ON)
option(SGB_ENABLE "build with SGB support" OFF)
option(GB_ENABLE_BUILTIN_PALETTE "build with builtin palettes" ON)

option(PLATFORM_SDL2 "" OFF)
option(PLATFORM_GAMECUBE "" OFF)
option(PLATFORM_LIBRETRO "" OFF)

find_program(CLANGD_INSTALLED clangd)
if (CLANGD_INSTALLED)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    message("found clangd: generating compile_commands.json to ${CMAKE_BINARY_DIR}/compile_commands.json")
else()
    message("cant find clangd: not generating compile_commands.json")
endif()

if (GB_DEV)
    set(GB_DEBUG ON)
endif()

if (NINTENDO_SWITCH)
    enable_language(CXX) # needed for linking
    set(BUILD_SHARED_LIBS OFF)
endif()

if (WIN32)
    # Using UNICODE with NFD currently creates random UTF-8 folders on the
    # filesystem.
    add_definitions(-DFTG_IGNORE_UNICODE)
endif()

if (PLATFORM_LIBRETRO)
    if (SWITCH)
        set(BUILD_SHARED_LIBS OFF)
    elseif (VITA)
        set(BUILD_SHARED_LIBS OFF)
    elseif (PSP)
        set(BUILD_SHARED_LIBS OFF)
    elseif (PS2)
        set(BUILD_SHARED_LIBS OFF)
    elseif (WIIU)
        set(BUILD_SHARED_LIBS OFF)
    elseif (WII)
        set(BUILD_SHARED_LIBS OFF)
    elseif (GAMECUBE)
        set(BUILD_SHARED_LIBS OFF)
    elseif (3DS)
        set(BUILD_SHARED_LIBS OFF)
    else()
        set(BUILD_SHARED_LIBS ON)
    endif()
endif()

set (ANDROID_TESTING OFF)

if (ANDROID OR ANDROID_TESTING)
    add_subdirectory(android)
endif()

add_subdirectory(src)
