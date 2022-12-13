find_package(Qt6 COMPONENTS Gui Widgets REQUIRED)

add_library(debug_util debug_util.cpp)
target_link_libraries(debug_util PUBLIC Qt6::Gui Qt6::Widgets)
target_include_directories(debug_util PUBLIC ./)

