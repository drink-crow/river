set(CMAKE_CXX_STANDARD 17)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Gui Widgets REQUIRED)
find_package(Drogon REQUIRED)

add_executable(river_test main.cpp http_server.cpp web_server.cpp qtview.cpp central_widget.cpp "main_window.cpp")
target_link_libraries(river_test PRIVATE river Qt6::Gui Qt6::Widgets Drogon::Drogon)
target_include_directories(river_test PRIVATE ./)

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
