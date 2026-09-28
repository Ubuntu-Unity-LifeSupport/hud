
add_definitions(
	-pedantic
	-Wall
	-Wextra
)

set(
	INTEGRATION_TESTS_SRC
	TestHud.cpp
)

add_executable(
	test-integration-tests
	${INTEGRATION_TESTS_SRC}
)

target_link_libraries(
	test-integration-tests
	test-utils
	hud-common
	hud-client
	${GTEST_LIBRARIES}
	${GMOCK_LIBRARIES}
	${QTDBUSTEST_LIBRARIES}
	${QTDBUSMOCK_LIBRARIES}
)

qt5_use_modules(test-integration-tests
	Test
)

add_hud_test(
	test-integration-tests
)
