cmake_minimum_required(VERSION 3.8.0)

option(RIVER_DEVELOP_TEST "river develop test" ON)


set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake")
project(river VERSION 0.0.0.1 LANGUAGES CXX)

if(RIVER_DEVELOP_TEST)
    add_subdirectory(debug_util)
endif()

add_subdirectory(src)
# add_subdirectory(third_parts/Clipper2/CPP)

if(RIVER_DEVELOP_TEST)
    add_subdirectory(test)
endif()
