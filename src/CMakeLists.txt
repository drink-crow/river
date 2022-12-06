
aux_source_directory(src SRC)

add_library(river ${SRC})
target_include_directories(river PUBLIC include PRIVATE src)
