cmake_minimum_required (VERSION 3.1)
project (congestion)

add_executable(congestion
    3a/reliable.c
    3a/reliable.h
    3a/rlib.c
    3a/rlib.h
    3a/sliding_window.c
    3a/sliding_window.h)

add_executable(sliding_window_test
    3a/reliable.c
    3a/reliable.h
    3a/rlib.c
    3a/rlib.h
    3a/sliding_window.c
    3a/sliding_window.h
    3a/sliding_window_tests.c)
