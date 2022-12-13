if (WIN32)
    add_compile_options(/utf-8)
endif()

set(CMAKE_CXX_STANDARD 17)

aux_source_directory(src SRC)

find_package(Boost REQUIRED)

add_library(river ${SRC})
target_include_directories(river PUBLIC include PRIVATE src ${Boost_INCLUDE_DIR})

target_compile_definitions(river PUBLIC RIVER_DEVELOP_TEST)
if(RIVER_DEVELOP_TEST)
    message(STATUS "develop mode")
    target_link_libraries(river PUBLIC debug_util)
endif()
