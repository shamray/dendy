cmake_minimum_required(VERSION 3.24)

project("NEM0")

find_package(Catch2 3 REQUIRED)
find_package(SDL2 REQUIRED)

include(Catch)
enable_testing()

add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(tools)