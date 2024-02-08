cmake_minimum_required(VERSION 3.14)
project(tz_1)

set(CMAKE_C_STANDART 11)

add_compile_definitions(DOUBLE)
add_executable(main_double tz_1.c)


add_executable(main_float tz_1.c)