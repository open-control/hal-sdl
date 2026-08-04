cmake_minimum_required(VERSION 3.29)

project(open_control_hal_sdl LANGUAGES CXX)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/OpenControlHalSdlSources.cmake")

add_library(oc_hal_sdl INTERFACE)
target_sources(oc_hal_sdl INTERFACE ${OC_HAL_SDL_SOURCES})
target_include_directories(oc_hal_sdl
    INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_features(oc_hal_sdl INTERFACE cxx_std_17)
