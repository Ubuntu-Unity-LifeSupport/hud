
set(
	UNIT_TESTS_SRC
	TestConnection.cpp
	TestHudClient.cpp
	TestParam.cpp
	TestQuery.cpp
)

add_executable(
	test-libhud-client-unit-tests
	${UNIT_TESTS_SRC}
)

target_link_libraries(
	test-libhud-client-unit-tests
	test-utils
	hud-client
	${GTEST_LIBRARIES}
	${GMOCK_LIBRARIES}
	${QTDBUSTEST_LIBRARIES}
	${QTDBUSMOCK_LIBRARIES}
)

qt5_use_modules(
	test-libhud-client-unit-tests
	Test
)

add_hud_test(
	test-libhud-client-unit-tests
)

