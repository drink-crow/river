set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(skia REQUIRED )
find_package(Qt6 COMPONENTS Gui Widgets REQUIRED)

add_executable(river_test main.cpp)
target_link_libraries(river_test PRIVATE river skia::skia Qt6::Gui Qt6::Widgets)

if(WIN32)
    get_target_property(moc_path Qt6::moc IMPORTED_LOCATION)
    get_filename_component(QT_BIN_DIR ${moc_path} DIRECTORY)
    add_custom_target(INSTALL_Qt
        DEPENDS river_test
        COMMAND ${QT_BIN_DIR}/windeployqt.exe $<TARGET_FILE:river_test>
        )
    set_target_properties(river_test PROPERTIES
        WIN32_EXECUTABLE YES
        LINK_FLAGS "/ENTRY:mainCRTStartup"
    )
endif()