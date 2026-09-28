if(${ENABLE_BAMF})

if(${ENABLE_BAMF})
	list(APPEND UNIT_TESTS_SRC TestBamfWindowStack.cpp)
endif()

add_executable(
	test-window-stack-bridge-unit-tests
	${UNIT_TESTS_SRC}
)

target_link_libraries(
	test-window-stack-bridge-unit-tests
	test-utils
	window-stack-bridge
	${GTEST_LIBRARIES}
	${GMOCK_LIBRARIES}
	${QTDBUSTEST_LIBRARIES}
	${QTDBUSMOCK_LIBRARIES}
)

qt5_use_modules(
	test-window-stack-bridge-unit-tests
	Test
)

add_hud_test(
	test-window-stack-bridge-unit-tests
)

endif()
