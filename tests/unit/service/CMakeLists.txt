
set(
	UNIT_TESTS_SRC
	TestApplication.cpp
	TestApplicationList.cpp
	TestHudService.cpp
	TestItemStore.cpp
	TestQuery.cpp
	TestUsageTracker.cpp
	TestVoice.cpp
	TestWindow.cpp
)

add_executable(
	test-service-unit-tests
	${UNIT_TESTS_SRC}
)

qt5_use_modules(
	test-service-unit-tests
	Test
)

target_link_libraries(
	test-service-unit-tests
	test-utils
	hud-service
	${GTEST_LIBRARIES}
	${GMOCK_LIBRARIES}
	${QTDBUSTEST_LIBRARIES}
	${QTDBUSMOCK_LIBRARIES}
)

add_hud_test(
	test-service-unit-tests
)
