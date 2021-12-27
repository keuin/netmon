cmake_minimum_required(VERSION 3.0)
project(netmon C)

set(CMAKE_C_STANDARD 99)
add_compile_definitions(DEBUG)

add_executable(netmon netmon.c logging.c logging.h)
