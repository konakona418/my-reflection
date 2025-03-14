cmake_minimum_required(VERSION 3.29)
project(my_reflection)

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE INCLUDE "${CMAKE_SOURCE_DIR}/include/*.h")
include_directories("${CMAKE_SOURCE_DIR}/include")

file(GLOB_RECURSE TESTS "${CMAKE_SOURCE_DIR}/test/*.h")

add_executable(my_reflection main.cpp ${INCLUDE} ${TESTS})
