add_executable(
    qtgmenu-test-app
    MainWindow.cpp
    main.cpp
)

qt5_use_modules(
    qtgmenu-test-app
    Widgets
)

target_link_libraries(
    qtgmenu-test-app
    qtgmenu
)
