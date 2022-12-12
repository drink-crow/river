cmake_minimum_required(VERSION 3.8.0)

set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake")
project(river VERSION 0.0.0.1 LANGUAGES CXX)

add_subdirectory(src)
add_subdirectory(test)
