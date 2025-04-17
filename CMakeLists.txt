cmake_minimum_required(VERSION 3.13)

list(APPEND CMAKE_MODULE_PATH $ENV{DEVKITPRO}/cmake)
list(APPEND CMAKE_MODULE_PATH $ENV{DEVKITPRO}/cmake/Platform)
set(DKP_GBA_PLATFORM_LIBRARY libgba)

include(GBA)
include(NintendoGBA)

project(RtcTest LANGUAGES C ASM)

set(CMAKE_C_STANDARD 11)

add_executable(RtcTestMB main.c)
add_executable(RtcTestROM main.c)
gba_create_rom(RtcTestMB MULTIBOOT PAD)
# BPEE is Emerald's game code, use it for emu autodetection
gba_create_rom(RtcTestROM PAD GAMECODE BPEE)
