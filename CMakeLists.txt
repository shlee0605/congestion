cmake_minimum_required (VERSION 3.0)
project (congestion)

set(CMAKE_C_FLAGS_RELEASE "-DNDEBUG")
set(CMAKE_C_FLAGS_DEBUG "-ggdb")

add_executable(congestion
    3a/reliable.c
    3a/reliable.h
    3a/rlib.c
    3a/rlib.h
    3a/sliding_window.c
    3a/sliding_window.h)