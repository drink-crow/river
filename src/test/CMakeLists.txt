set(CMAKE_CXX_STANDARD 17)

add_executable(main main.cpp)
target_link_libraries(main PRIVATE river Clipper2)

if(WIN32)
    get_target_property(moc_path Qt6::moc IMPORTED_LOCATION)
    get_filename_component(QT_BIN_DIR ${moc_path} DIRECTORY)
    add_custom_target(INSTALL_main_Qt
        DEPENDS river_test
        COMMAND ${QT_BIN_DIR}/windeployqt.exe $<TARGET_FILE:main>
    )
endif()
