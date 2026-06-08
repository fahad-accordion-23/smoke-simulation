cmake_minimum_required(VERSION 3.15)
project(SmokeSimulation C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(SDL3 REQUIRED)

include_directories(src)

set(SOURCES
    src/arena.c
    src/main.c
)

add_executable(PlayGame ${SOURCES})

target_compile_options(PlayGame PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Wconversion
    -Wsign-conversion
    -Werror
)

target_link_libraries(PlayGame PRIVATE SDL3::SDL3)
