cmake_minimum_required(VERSION 3.29)
project(my_reflection)

set(CMAKE_CXX_STANDARD 17)

add_executable(my_reflection main.cpp
        simple_refl.h
        test_helper.h)
