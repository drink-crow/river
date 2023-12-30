cmake_minimum_required(VERSION 3.8.0)

option(RIVER_DEVELOP_TEST "river develop test" OFF)

if(WIN32)
    set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake")
endif()

project(river VERSION 0.0.0.1 LANGUAGES CXX)

if(RIVER_DEVELOP_TEST)
    add_subdirectory(debug_util)
endif()

add_subdirectory(src)

if(RIVER_DEVELOP_TEST)
    add_subdirectory(test)
endif()
