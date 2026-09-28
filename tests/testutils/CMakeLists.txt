
add_definitions(
	-pedantic
	-Wall
	-Wextra
)

set(
	TEST_UTILS_SRC
	MockHudService.cpp
	RawDBusTransformer.cpp
	main.cpp
)

qt5_add_dbus_interface(
	TEST_UTILS_SRC
	${HUD_TEST_XML}
	HudTestInterface
)

add_library(
	test-utils
	SHARED
	${TEST_UTILS_SRC}
)

qt5_use_modules(
	test-utils
	Core
	Widgets
)

target_link_libraries(
	test-utils
	hud-common
)
