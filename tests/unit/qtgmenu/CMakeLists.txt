set(
    UNIT_TESTS_SRC
    TestQtGMenu.cpp
)

add_executable(
    test-qtgmenu-unit-tests
    ${UNIT_TESTS_SRC}
)

qt5_use_modules(
    test-qtgmenu-unit-tests
    Test
)

add_library(
    qtgmenu-test-main
    main.cpp
)

target_link_libraries(
    test-qtgmenu-unit-tests
    qtgmenu-test-main
    qtgmenu
    ${GIO2_LIBRARIES}
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${QTDBUSTEST_LIBRARIES}
    ${QTDBUSMOCK_LIBRARIES}
)

add_hud_test(
    test-qtgmenu-unit-tests
)
