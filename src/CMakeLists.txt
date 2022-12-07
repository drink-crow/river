
aux_source_directory(src SRC)

find_package(Boost REQUIRED)

add_library(river ${SRC})
target_include_directories(river PUBLIC include PRIVATE src ${Boost_INCLUDE_DIR})
