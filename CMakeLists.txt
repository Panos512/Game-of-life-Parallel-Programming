cmake_minimum_required(VERSION 3.6)
project(the_game_of_life)

set(CMAKE_C_STANDARD 99)

set(SOURCE_FILES main.c game_of_life.c)
add_executable(the_game_of_life ${SOURCE_FILES})
include_directories(/usr/include/mpi/)